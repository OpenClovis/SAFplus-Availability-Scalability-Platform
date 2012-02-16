#include <asm/types.h>
#include <ucontext.h>

#define sc_pc pc
#define sc_regs gregs

typedef struct pmap_line {
    unsigned long vm_start;     /* Our start address within vm_mm. */
    unsigned long vm_end;       /* The first byte after our
                                   end address within vm_mm. */
    char perm[5];               /* permission */
    unsigned long vm_pgoff;     /* Offset (within vm_file) in PAGE_SIZE units,
                                   *not* PAGE_CACHE_SIZE */
    char dev[6];                /* device name */
    unsigned long ino;              /* innode number */
    struct pmap_line *next;
} pmap_line_t;

static pmap_line_t *pmap_line_head=NULL;

static void free_pmap_line()
{
    pmap_line_t *line=NULL;
    while((line=pmap_line_head) != NULL)
    {
        pmap_line_head=pmap_line_head->next;
        free(line);
    }
}

static void getpmaps(size_t pid)
{

    FILE *f;
    char buf[4096+100]={0};
    pmap_line_t *pmap_line_tail=NULL;
    pmap_line_t *line=NULL;
    char fname [50]={0};

    sprintf(fname, "/proc/%ld/maps", (long)pid);
    f = fopen(fname, "r");
    if(!f)
    {
        return;
    }

    while(!feof(f))
    {
        if(fgets(buf, sizeof(buf), f) == 0)
            break;
        line=(pmap_line_t*)malloc(sizeof(pmap_line_t));
        memset(line,sizeof(pmap_line_t),0);

        sscanf(buf, "%lx-%lx %4s %lx %5s %lu", &line->vm_start, &line->vm_end,
                line->perm, &line->vm_pgoff, line->dev, &line->ino);
        line->next=NULL;

        if(!pmap_line_head)
        {
            pmap_line_head=line;
        }
        if(pmap_line_tail)
        {
            pmap_line_tail->next=line;
        }
        pmap_line_tail = line;
    }
    line=pmap_line_head;
    while(line)
    {
        printf("%08lx-%08lx %s %08lx %s %lu\n",line->vm_start,line->vm_end,
                line->perm,line->vm_pgoff,line->dev,line->ino);
        line=line->next;
    }
    fclose(f);
}

static int canReadAddr(unsigned long addr)
{
    pmap_line_t *line=pmap_line_head;
    if(!pmap_line_head)
    {
        return 0;
    }

    while(line)
    {
        if(line->perm[0] == 'r' &&
                addr >= line->vm_start && addr <=line->vm_end)
        {
            return 1;
        }
        line=line->next;
    }
    printf("cannot read address %lu\n",addr);
    return 0;

}

/**PROC+**********************************************************************/
/* Name:      backtrace_mips                                              */
/*                                                                           */
/* Purpose:   standard backtrace function provided by glic does NOT support  */
/*            MIPS architecture. This function provids same functionality for*/
/*            MIPS, retreive the calling function stack pointer address based*/
/*            on current PC.                                                 */
/*                                                                           */
/* Returns:   Nothing.                                                       */
/*                                                                           */
/* Params:    IN/OUT     buffer   - buffer to hold text address returned     */
/*            IN/OUT     buffer   - buffer to hold text function address     */
/*                                  returned                                 */
/*                                                                           */
/*            IN         size     - buffer size                              */
/*                                                                           */
/*            IN         uc       -MIPS ucontext structure returned from     */
/*                                 kernel, holding registers when signal     */
/*                                 occurs.                                   */
/*                                                                           */
/**PROC-**********************************************************************/
static int backtrace_mips(void ** buffer,void ** func,
                          int size, ucontext_t *uc)
{
    unsigned long *addr = NULL;
    unsigned long *pc=NULL;
    unsigned long *ra=NULL;
    unsigned long *sp=NULL;
    size_t ra_offset=0;
    size_t stack_size=0;
    size_t depth=0;
    int first=0;

    if(size == 0){
        return 0;
    }

    if(size<0 || !uc){
        return -1;
    }

    getpmaps(getpid());

    //get current $pc, $ra and $sp
    pc=(unsigned long*)(unsigned long)(greg_t)uc->uc_mcontext.sc_pc;
    ra=(unsigned long*)(unsigned long)(greg_t)(uc->uc_mcontext.sc_regs[31]);
    sp=(unsigned long*)(unsigned long)(greg_t)(uc->uc_mcontext.sc_regs[29]);

    if(canReadAddr((unsigned long)pc))
    {
        depth=1;
        if(buffer)
            buffer[0] = pc;
    }
    else
    {
        goto out_free;
    }

    //scanning to find the size of the current stack-frame
    ra_offset = stack_size = 0;

    addr=pc;
    first=1;

    while(1)
    {
        //printf("addr:%08lx, pc:%08lx, ra content:%lx\n", addr, pc, *ra);
        if(!canReadAddr((unsigned long)addr))
        {
            break;
        }
       
        if(*addr == 0x1000ffff)
        {
            break;
        }

        switch(*addr & 0xffff0000)
        {
        case 0x23bd0000:
            /* 0x23bdxxxx: ADDI SP, SP, xxxx
               ADDIU -- Add immediate (with overflow)
               Description:
               Adds a register and a sign-extended immediate value and
               stores the result in a register
              
               Operation:
               $t = $s + imm; advance_pc (4);
              
               Syntax:
               addi $t, $s, imm
              
               Encoding:
               0010 00ss ssst tttt iiii iiii iiii iiii
             
               register : $29 sp
             
            */
        case 0x27bd0000:
            /* 0x27bdxxxx: ADDIU SP, SP, xxxx
               ADDIU -- Add immediate unsigned (no overflow)
               Description:
               Adds a register and a sign-extended immediate value and
               stores the result in a register
              
               Operation:
               $t = $s + imm; advance_pc (4);
              
               Syntax:
               addiu $t, $s, imm
              
               Encoding:
               0010 01ss ssst tttt iiii iiii iiii iiii
             
               register : $29 sp
             
            */
            stack_size = abs((short)(*addr & 0xffff));
            //printf("----get addiu try to jump back---\n");
            //printf("old ra:%08lx, ra_offset:%d, old sp:%08lx, sp_offset:%d \n", ra, ra_offset, sp, stack_size);
            //printf("old ra content:%08lx\n", *ra);
            if(!first && (!ra_offset || !stack_size))
            {
                //there's no return address or stack size,
                //reach the calling stack top
                break;
            }
            else {

                //first level function, there might no ra/sp from text
                //we might use those from regs
                func[depth-1]=addr;

                if(depth == size)
                {
                    break;
                }

                if(ra_offset && !canReadAddr((unsigned long)sp+ra_offset))
                {
                    break;
                }
                if(ra_offset){
                    ra = (unsigned long*)(*(unsigned long*)
                                          ((unsigned long)sp+ra_offset));
                }
                if(stack_size){
                    sp = (unsigned long*)
                        ((unsigned long)sp + stack_size);
                }
                //printf("new ra:%08lx, new sp:%08lx \n", ra, sp);
                //printf("---jumping...---\n");
                if(buffer)
                    buffer[depth] = ra;
                ++depth;
                stack_size=0;
                ra_offset=0;
                addr=ra;
            }
            first=0;
            //                --addr;
            break;

        case 0xafbf0000:
            /*0xafbfxxxx : sw ra ,xxxx(sp)
              SW -- Store word
              Description:
              The contents of $t is stored at the specified address.
              
              Operation:
              MEM[$s + offset] = $t; advance_pc (4);
              
              Syntax:
              sw $t, offset($s)
              
              Encoding:
              1010 11ss ssst tttt iiii iiii iiii iiii
             
              register $31 : ra ; $29 : sp
             
            */
            ra_offset = (short)(*addr & 0xffff);
            --addr;
            break;

        case 0xffbf0000:
            /*0xffbfxxxx : sd ra ,xxxx(sp)
              SD-- Store double word
              Description:
              The contents of $t is stored at the specified address.
              
              Operation:
              MEM[$s + offset] = $t; advance_pc (4);
              
              Syntax:
              sd $t, offset($s)
              
              Encoding:
              1010 11ss ssst tttt iiii iiii iiii iiii
             
              register $31 : ra ; $29 : sp
             
            */
            ra_offset = (short)(*addr & 0xffff);
            --addr;
            break;

            //            case 0x3c1c0000:
            //            /*  3c1cxxxx    lui gp xxxx
            //                LUI -- Load upper immediate
            //                Description:
            //                 The immediate value is shifted left 16 bits and stored
            //                 in the register. The lower 16 bits are zeroes.
            //                
            //                Operation:
            //                 $t = (imm << 16); advance_pc (4);
            //                
            //                Syntax:
            //                 lui $t, imm
            //                
            //                Encoding:
            //                 0011 11-- ---t tttt iiii iiii iiii iiii
            //               
            //                registers: $28 : gp
            //           */
            //                if(!first && (!ra_offset || !stack_size))
            //                {
            //                    //there's no return address or stack size,
            //                    //reach the calling stack top
            //                    return depth;
            //                }
            //                else{
            //
            //                    //first level function, there might no ra/sp from text
            //                    //we might use those from regs
            //                    func[depth-1]=addr;
            //
            //                    if(depth == size)
            //                    {
            //                        return depth;
            //                    }
            //
            //                    if(ra_offset && !canReadAddr((unsigned long long)sp+ra_offset))
            //                    {
            //                        return depth;
            //                    }
            //                    if(ra_offset){
            //                        ra = (unsigned long*)(*(unsigned long long*)(
            //                                    (unsigned long long)sp+ra_offset));
            //                    }
            //                    if(stack_size){
            //                        sp = (unsigned long long*) (
            //                                (unsigned long long)sp + stack_size);
            //                    }
            //                    if(buffer)
            //                        buffer[depth] = ra;
            //                    ++depth;
            //                    stack_size=0;
            //                    ra_offset=0;
            //                    addr=ra;
            //                }
            //                first=0;
            //                break;

        default:
            --addr;
        }
    }

    out_free:
    free_pmap_line();

    return depth;
}
