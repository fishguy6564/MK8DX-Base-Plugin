#pragma once

#include "pattern.hpp"
#include "range.hpp"
#include "types.h"

#include "thread/static_thread.hpp"

#include <queue>
#include <string>
#include <functional>

namespace base::memory
{
	template <u32 T>
	class batch
	{
	private:
		struct entry
		{
			std::string m_name;
			pattern m_pattern;
			std::function<void (handle)> m_callback;
		};
		
		struct task_arg
		{
			range m_range;
			entry m_entry;
		};

		const u32 m_maxThreads;

		std::vector<entry> m_entries;
		std::queue<exl::thread::StaticThread<PAGE_SIZE>*>* m_active;

		static void task_func(void * p)
		{		
			task_arg* arg = static_cast<task_arg *>(p);
			std::string fmt = "";

			if (base::memory::handle handle = arg->m_range.scan(arg->m_entry.m_pattern))
			{
				if (arg->m_entry.m_callback)
				{
					std::invoke(std::move(arg->m_entry.m_callback), handle);
					return;
				}
			}

			// fmt = std::format("Failed to find batch entry '{}'.", arg->m_entry.m_name);
			// Logging.Log(fmt);
		}

		/*
		* Assigns a job to an available thread by checking it out
		* from the thread pool.
		*
		* @param jobs A queue of byte pattern tasks to pull from
		* and assign to a thread.
		*/
		void checkoutThreads(std::queue<task_arg*>& jobs)
		{
			while(this->m_active->size() < m_maxThreads)
			{
				if(jobs.empty()) break;

				// Dequeue next job in queue.
				auto job = jobs.front();
				jobs.pop();

				// Dequeue an available thread.
				auto thread = new exl::thread::StaticThread<PAGE_SIZE>();
				thread->Create(task_func, job);

				// Enqueue an active thread in the active queue.
				this->m_active->push(thread);
			}
		}

		/*
		* Starts each checked out thread.
		*/
		void startThreads()
		{
			std::queue<exl::thread::StaticThread<PAGE_SIZE>*> temp;

			// Iterate through the active queue and start each thread.
			while(!this->m_active->empty())
			{
				auto thread = this->m_active->front();
				this->m_active->pop();

				thread->Start();
				temp.push(thread);
			}

			// Repopulate popped threads tot he active queue.
			while(!temp.empty())
			{
				this->m_active->push(temp.front());
				temp.pop();
			}
		}

		/*
		* Returned completed threads to the available thread pool.
		*/
		void returnThreads()
		{
			while(!this->m_active->empty())
			{
				auto thread = this->m_active->front();
				this->m_active->pop();
				
				thread->Wait();
				delete thread;
			}
		}

		/*
		* Processes a set of tasks through the threadpool pipeline.
		*
		* @param jobs A queue of byte pattern tasks to process.
		*/
		void process(std::queue<task_arg*>& jobs)
		{
			while(true)
			{
				if(jobs.empty()) break;

				// Thread pool pipeline.
				this->checkoutThreads(jobs);
				this->startThreads();
				this->returnThreads();
			}
		}

	public:
		explicit batch() : m_maxThreads(T)
		{
			this->m_active = new std::queue<exl::thread::StaticThread<PAGE_SIZE>*>();
		}

		~batch()
		{
			delete this->m_active;
		}

		/*
		* Adds a byte pattern to process when scanning.
		*
		* @param name The name associated with the byte pattern.
		* @param pattern The pattern object containing the byte pattern itself.
		* @param callback The callback function to call when the the pattern is found.
		*/
		void add(std::string name, pattern pattern, std::function<void (handle)> callback)
		{
			this->m_entries.emplace_back(std::move(name), std::move(pattern), std::move(callback));
		}

		/*
		* Scans the provided memory region with the set of byte patterns.
		*
		* @param range A range object containing memory information on the main memory region.
		*/
		void run(range range)
		{
			std::queue<task_arg*> jobs;
			std::vector<task_arg> job_storage;

			job_storage.reserve(m_entries.size());

			for (auto const& entry : this->m_entries)
			{
				job_storage.emplace_back(range, entry);
				jobs.push(&job_storage.back());
			}
			
			this->process(jobs);
			m_entries.clear();
		}
	};
}
