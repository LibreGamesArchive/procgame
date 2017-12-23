static char const* program_name = "program";

#if defined(__linux__)
    #include <signal.h>
    #include <execinfo.h>
    #include <unistd.h>
    #include <err.h>

    #define MAX_STACK_FRAMES 64
    static void *stack_traces[MAX_STACK_FRAMES];
    void posix_print_stack_trace(FILE* crashlog)
    {
        int i, trace_size = 0;
        trace_size = backtrace(stack_traces, MAX_STACK_FRAMES);
        for (i = 0; i < trace_size; ++i) {
            fprintf(crashlog, "%d: %p\n", i, stack_traces[i]);
        }
    }

    void posix_signal_handler(int sig, siginfo_t *siginfo, void *context)
    {
        (void)context;
        FILE* crashlog = fopen("crashlog.txt", "w");
        if(!crashlog) {
            printf("Couldn't write crashlog.txt\n");
            exit(1);
        } else {
            printf("Yikes! Report saved to crashlog.txt\n");
        }
        switch(sig)
        {
        case SIGSEGV:
            fputs("Caught SIGSEGV: Segmentation Fault\n", crashlog);
            break;
        case SIGFPE:
            switch(siginfo->si_code)
            {
            case FPE_INTDIV:
                fputs("Caught SIGFPE: (integer divide by zero)\n", crashlog);
                break;
            case FPE_INTOVF:
                fputs("Caught SIGFPE: (integer overflow)\n", crashlog);
                break;
            case FPE_FLTDIV:
                fputs("Caught SIGFPE: (floating-point divide by zero)\n", crashlog);
                break;
            case FPE_FLTOVF:
                fputs("Caught SIGFPE: (floating-point overflow)\n", crashlog);
                break;
            case FPE_FLTUND:
                fputs("Caught SIGFPE: (floating-point underflow)\n", crashlog);
                break;
            case FPE_FLTRES:
                fputs("Caught SIGFPE: (floating-point inexact result)\n", crashlog);
                break;
            case FPE_FLTINV:
                fputs("Caught SIGFPE: (floating-point invalid operation)\n", crashlog);
                break;
            case FPE_FLTSUB:
                fputs("Caught SIGFPE: (subscript out of range)\n", crashlog);
                break;
            default:
                fputs("Caught SIGFPE: Arithmetic Exception\n", crashlog);
                break;
            }
        case SIGILL:
            switch(siginfo->si_code)
            {
            case ILL_ILLOPC:
                fputs("Caught SIGILL: (illegal opcode)\n", crashlog);
                break;
            case ILL_ILLOPN:
                fputs("Caught SIGILL: (illegal operand)\n", crashlog);
                break;
            case ILL_ILLADR:
                fputs("Caught SIGILL: (illegal addressing mode)\n", crashlog);
                break;
            case ILL_ILLTRP:
                fputs("Caught SIGILL: (illegal trap)\n", crashlog);
                break;
            case ILL_PRVOPC:
                fputs("Caught SIGILL: (privileged opcode)\n", crashlog);
                break;
            case ILL_PRVREG:
                fputs("Caught SIGILL: (privileged register)\n", crashlog);
                break;
            case ILL_COPROC:
                fputs("Caught SIGILL: (coprocessor error)\n", crashlog);
                break;
            case ILL_BADSTK:
                fputs("Caught SIGILL: (internal stack error)\n", crashlog);
                break;
            default:
                fputs("Caught SIGILL: Illegal Instruction\n", crashlog);
                break;
            }
            break;
        case SIGTERM:
            fputs("Caught SIGTERM: a termination request was sent to the program\n", crashlog);
            break;
        case SIGABRT:
            fputs("Caught SIGABRT: usually caused by an abort() or assert()\n", crashlog);
            break;
        default:
            break;
        }
        fflush(crashlog);
        posix_print_stack_trace(crashlog);
        exit(1);
    }

    void set_signal_handler()
    {
        struct sigaction sig_action = {};
        sig_action.sa_sigaction = posix_signal_handler;
        sigemptyset(&sig_action.sa_mask);
        sig_action.sa_flags = SA_SIGINFO;
        if(sigaction(SIGSEGV, &sig_action, NULL) != 0) { err(1, "sigaction"); }
        if(sigaction(SIGFPE,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
        if(sigaction(SIGILL,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
        if(sigaction(SIGTERM, &sig_action, NULL) != 0) { err(1, "sigaction"); }
        if(sigaction(SIGABRT, &sig_action, NULL) != 0) { err(1, "sigaction"); }
    }
#elif defined(_WIN32)
    #include <windows.h>
    #include <imagehlp.h>

    void windows_print_stacktrace(CONTEXT* context, FILE* crashlog)
    {
        SymInitialize(GetCurrentProcess(), 0, TRUE);
        STACKFRAME frame = { 0 };
        /* setup initial stack frame */
        #if defined(_WIN64)
            frame.AddrPC.Offset         = context->Rip;
            frame.AddrPC.Mode           = AddrModeFlat;
            frame.AddrStack.Offset      = context->Rsp;
            frame.AddrStack.Mode        = AddrModeFlat;
            frame.AddrFrame.Offset      = context->Rbp;
            frame.AddrFrame.Mode        = AddrModeFlat;
            int f = 0;
            while(StackWalk(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(),
                            GetCurrentThread(), &frame, context, 0,
                            SymFunctionTableAccess, SymGetModuleBase, 0)) {
                fprintf(crashlog, "%d: %p\n", f, (void*)frame.AddrPC.Offset);
                ++f;
            }
        #elif defined(_WIN32)
            frame.AddrPC.Offset         = context->Eip;
            frame.AddrPC.Mode           = AddrModeFlat;
            frame.AddrStack.Offset      = context->Esp;
            frame.AddrStack.Mode        = AddrModeFlat;
            frame.AddrFrame.Offset      = context->Ebp;
            frame.AddrFrame.Mode        = AddrModeFlat;
            int f = 0;
            while(StackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(),
                            GetCurrentThread(), &frame, context, 0,
                            SymFunctionTableAccess, SymGetModuleBase, 0)) {
                fprintf(crashlog, "%d: %p\n", f, (void*)frame.AddrPC.Offset);
                ++f;
            }
        #endif
        SymCleanup( GetCurrentProcess() );
    }

    LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS * ExceptionInfo)
    {
        FILE* crashlog = fopen("crashlog.txt", "w");
        if(!crashlog) {
            printf("Couldn't write crashlog.txt\n");
            exit(1);
        } else {
            printf("Yikes! Crash report saved to crashlog.txt\n");
        }
        switch(ExceptionInfo->ExceptionRecord->ExceptionCode)
        {
        case EXCEPTION_ACCESS_VIOLATION:
            fputs("Error: EXCEPTION_ACCESS_VIOLATION\n", crashlog);
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            fputs("Error: EXCEPTION_ARRAY_BOUNDS_EXCEEDED\n", crashlog);
            break;
        case EXCEPTION_BREAKPOINT:
            fputs("Error: EXCEPTION_BREAKPOINT\n", crashlog);
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            fputs("Error: EXCEPTION_DATATYPE_MISALIGNMENT\n", crashlog);
            break;
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            fputs("Error: EXCEPTION_FLT_DENORMAL_OPERAND\n", crashlog);
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            fputs("Error: EXCEPTION_FLT_DIVIDE_BY_ZERO\n", crashlog);
            break;
        case EXCEPTION_FLT_INEXACT_RESULT:
            fputs("Error: EXCEPTION_FLT_INEXACT_RESULT\n", crashlog);
            break;
        case EXCEPTION_FLT_INVALID_OPERATION:
            fputs("Error: EXCEPTION_FLT_INVALID_OPERATION\n", crashlog);
            break;
        case EXCEPTION_FLT_OVERFLOW:
            fputs("Error: EXCEPTION_FLT_OVERFLOW\n", crashlog);
            break;
        case EXCEPTION_FLT_STACK_CHECK:
            fputs("Error: EXCEPTION_FLT_STACK_CHECK\n", crashlog);
            break;
        case EXCEPTION_FLT_UNDERFLOW:
            fputs("Error: EXCEPTION_FLT_UNDERFLOW\n", crashlog);
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            fputs("Error: EXCEPTION_ILLEGAL_INSTRUCTION\n", crashlog);
            break;
        case EXCEPTION_IN_PAGE_ERROR:
            fputs("Error: EXCEPTION_IN_PAGE_ERROR\n", crashlog);
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            fputs("Error: EXCEPTION_INT_DIVIDE_BY_ZERO\n", crashlog);
            break;
        case EXCEPTION_INT_OVERFLOW:
            fputs("Error: EXCEPTION_INT_OVERFLOW\n", crashlog);
            break;
        case EXCEPTION_INVALID_DISPOSITION:
            fputs("Error: EXCEPTION_INVALID_DISPOSITION\n", crashlog);
            break;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            fputs("Error: EXCEPTION_NONCONTINUABLE_EXCEPTION\n", crashlog);
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            fputs("Error: EXCEPTION_PRIV_INSTRUCTION\n", crashlog);
            break;
        case EXCEPTION_SINGLE_STEP:
            fputs("Error: EXCEPTION_SINGLE_STEP\n", crashlog);
            break;
        case EXCEPTION_STACK_OVERFLOW:
            fputs("Error: EXCEPTION_STACK_OVERFLOW\n", crashlog);
            break;
        default:
            fputs("Error: Unrecognized Exception\n", crashlog);
            break;
        }
        fflush(crashlog);
        /* If this is a stack overflow then we can't walk the stack, so just show
          where the error happened */
        if (EXCEPTION_STACK_OVERFLOW != ExceptionInfo->ExceptionRecord->ExceptionCode) {
            windows_print_stacktrace(ExceptionInfo->ContextRecord, crashlog);
        } else {
            #if defined(_WIN64)
            fprintf(crashlog, "Stack overflow: %p\n", ExceptionInfo->ContextRecord->Rip);
            #elif defined(_WIN32)
            fprintf(crashlog, "Stack overflow: %p\n", ExceptionInfo->ContextRecord->Eip);
            #endif
        }
        exit(1);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void set_signal_handler()
    {
        SetUnhandledExceptionFilter(windows_exception_handler);
    }
#endif

