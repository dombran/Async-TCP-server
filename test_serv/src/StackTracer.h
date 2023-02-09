/*
 * StackTracer.h
 *
 *  Created on: Sep 19, 2019
 *      Author: dombran
 */

#ifndef STACKTRACER_H_
#define STACKTRACER_H_

#include "stdafx.h"

class StackTracer
{
    friend class CallHolder;

public:

    static StackTracer& i()
    {
        static StackTracer s;
        return s;
    }

    std::string getStackTrace() const
    {
        std::stringstream ss;

        std::lock_guard<std::mutex> guard(m_readMutex);
        for (auto mapIterator = m_data.begin(), mapEnd = m_data.end();
             mapIterator != mapEnd;
             ++mapIterator)
        {
            ss << "Thread: 0x" << std::hex << mapIterator->first << std::dec << std::endl;

            for (auto listIterator = mapIterator->second.begin(), listEnd = mapIterator->second.end();
                 listIterator != listEnd;
                 ++listIterator)
                ss << listIterator->file << ':' << listIterator->line << " -> " << listIterator->name << std::endl;
            ss << std::endl;
        }

        return ss.str();
    }

private:
    void push(const std::string &name, const char *file, int line, std::thread::id thread_id)
    {
        m_data[thread_id].push_front({name, file, line});
    }

    void pop(std::thread::id thread_id)
    {
        m_data[thread_id].pop_front();
    }

    struct CallData
    {
        std::string name;
        const char *file;
        int line;
    };

    StackTracer() :
            m_data()
    {}

    mutable std::mutex m_readMutex;
    std::map<std::thread::id, std::list<CallData> > m_data;
};

class CallHolder
{
public:
    CallHolder(const std::string &name, const char *file, int line, std::thread::id thread_id)
    {
        StackTracer::i().push(name, file, line, thread_id);
        m_id = thread_id;
    }

    ~CallHolder()
    {
        StackTracer::i().pop(m_id);
    }

private:
    std::thread::id m_id;
};

#define CLASS_IMPL(func_name, args)\
func_name args\
{\
    CallHolder __f(type_name<decltype(*this)>() + "::" + #func_name + #args, __FILE__, __LINE__, std::this_thread::get_id());


#define MEM_IMPL(func_name, args)\
func_name args\
{\
    CallHolder __f("" #func_name #args "", __FILE__, __LINE__, std::this_thread::get_id());

#define THROW(exception, explanation)\
throw exception(explanation + std::string("\n\rStack trace:\n\r") + StackTracer::i().getStackTrace());


#endif /* STACKTRACER_H_ */
