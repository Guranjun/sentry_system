#ifndef __STORAGE_VIDEO_HPP
#define __STORAGE_VIDEO_HPP

#ifdef __cplusplus
extern "C" {
#endif
void* storage_video_thread(void* arg);
void storage_thread_wakeup(void);
#ifdef __cplusplus
}
#endif


#endif // __STORAGE_VIDEO_HPP