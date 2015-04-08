#ifndef _PWM_SYNC_H_
#define _PWM_SYNC_H_

#define PWM_IOC_MAGIC 'k'

#define PWM_IOC_START    _IO(PWM_IOC_MAGIC, 1)
#define PWM_IOC_STOP     _IO(PWM_IOC_MAGIC, 2)
#define PWM_IOC_SET_FREQ _IOW(PWM_IOC_MAGIC, 3, unsigned long)
#define PWM_IOC_NR 3


#endif