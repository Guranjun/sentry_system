#ifndef ALARM_HPP
#define ALARM_HPP

#ifdef __cplusplus
extern "C"{
#endif
void* process_image_thread(void* arg) ;
void alarm_thread_wakeup(void);
#ifdef __cplusplus
}
#endif




#endif // ALARM_HPP