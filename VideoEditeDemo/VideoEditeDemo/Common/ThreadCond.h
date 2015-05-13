//
//  ThreadCond.h
//  MovieEditor
//
//  Created by lyq on 14-3-12.
 
//

#ifndef MovieEditor_ThreadCond_h
#define MovieEditor_ThreadCond_h

#include <pthread.h>

class ThreadCond
{
public:
    ThreadCond()
    {
        pthread_cond_init(&m_cond, NULL);
        pthread_mutex_init(&m_mutex, NULL);
    }
    
    ~ThreadCond()
    {
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }
    
    void Lock()
    {
        pthread_mutex_lock(&m_mutex);
    }
    
    void Unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }
    
    void Wait()
    {
        pthread_cond_wait(&m_cond, &m_mutex);
    }
    
    void Signal()
    {
        pthread_cond_signal(&m_cond);
    }
    
private:
    pthread_mutex_t m_mutex;
    pthread_cond_t  m_cond;
};

class AutoLock
{
    ThreadCond *m_cond;
public:
    AutoLock(ThreadCond *cond)
    {
        m_cond = cond;
        if (m_cond) {
            m_cond->Lock();
        }
    }
    
    ~AutoLock()
    {
        if (m_cond) {
            m_cond->Unlock();
        }
    }
    
};

template<class T>
class SimpleThread
{
    typedef void (T::*thread_entry)();
    T               *m_target;
    thread_entry    m_entry;
public:
    SimpleThread(T *target, thread_entry entry, pthread_t *p_pt=NULL)
    {
        m_target    = target;
        m_entry     = entry;
        
        pthread_t pt;
        if (p_pt==NULL) {
            p_pt = &pt;
        }
        pthread_create(p_pt, NULL, SimpleThread<T>::thread_proc, this);
    }
    
private:
    static void* thread_proc(void *parm)
    {
        SimpleThread *p_this = (SimpleThread*)parm;
        
        T               *target = p_this->m_target;
        thread_entry    entry   = p_this->m_entry;
        
        (target->*entry)();
        
        delete p_this;
        
        return NULL;
    }
};

#endif
