obj-y += delay_tsc.o

# default CONFIG_TSC_X86RDTSC_CALIBRATE_WITH_TIMER2=0
# if CONFIG_UDELAY_TSC
# 	default CONFIG_HAVE_INIT_TIMER=1
# 	object delay_tsc.o
# end