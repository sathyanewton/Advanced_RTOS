// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// code for lab 4 -M.G.
    uint32_t pte = uvpt[PGNUM(addr)];

	if ((pte & ( PTE_W | PTE_COW)) == 0)
    {
   		panic("ERROR: Wrong permissions!!");
    }

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	// code for lab 4-M.G

	addr = ROUNDDOWN(addr, PGSIZE);

	if (sys_page_alloc(0, PFTEMP, PTE_W|PTE_U|PTE_P) < 0)
	{
    	panic("ERROR in sys_page_alloc");
    }

	memmove(PFTEMP, addr, PGSIZE);

	if (sys_page_map(0, PFTEMP, 0, addr, PTE_W|PTE_U|PTE_P) < 0)
    {
		panic("ERROR in sys_page_map");
    }
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	// code for lab 4 & 5 -M.G
    //	panic("duppage not implemented");    
    int perm = PTE_P|PTE_U;
	void *va = (void *)(pn * PGSIZE);

    if (uvpt[pn] & PTE_SHARE)
    {
		return  sys_page_map(0, va, envid, va, uvpt[pn] & PTE_SYSCALL);
    }

	if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW))
	{
    	perm |= PTE_COW;
    }

	if (sys_page_map(0, va, envid, va, perm) < 0)
    {
		panic("ERROR in sys_page_map!!");
    }

	if (sys_page_map(0, va, 0, va, perm) < 0)
	{
    	panic("ERROR in sys_page_map!!");
    }

    return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// code for lab 4 - M.G
    //	panic("fork not implemented");

    // LAB 4: Your code here.
	set_pgfault_handler(pgfault);

	envid_t envid;

	if ((envid = sys_exofork()) < 0)
    {
		panic("ERROR: in sys_exofork");
    }

	if (envid == 0)
    {
		thisenv = (struct Env *)envs + ENVX(sys_getenvid());
		return 0;
	}

	uint32_t page_number;
	uint32_t i,j;

	for (i = 0; i < PDX(UTOP); i++)
    {
		if (uvpd[i] & PTE_P)
        {
			for (j = 0; j < NPTENTRIES; j++)
            {
				page_number = i* NPTENTRIES + j;
				if (page_number == PGNUM(UXSTACKTOP - PGSIZE)) 
                {
					break;
				}
				
				if (uvpt[page_number] & PTE_P)
                {
					duppage(envid, page_number);
				}
			}
		}
	}

	if ((sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
    {
    	panic("ERROR in sys_page_alloc!!");
    }

	if (sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall) < 0)
    {
		panic("ERROR in sys_env_set_pgfault_upcall!!");
    }

	if (sys_env_set_status(envid, ENV_RUNNABLE) < 0)
	{
    	panic("ERROR in sys_env_set_status!!");
    }
	return envid;
}


// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
