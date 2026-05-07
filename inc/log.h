#ifndef __LOG_H
#define __LOG_H

void* logger_process_thread(void* arg);
void logger_thread_wakeup(void);
#endif // __LOG_H