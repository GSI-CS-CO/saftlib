#ifndef SAFTBUS_LOOP_HPP_
#define SAFTBUS_LOOP_HPP_

#include <memory>
#include <chrono>
// #include <sigc++/sigc++.h>
#include <functional>
#include <array>

#include <poll.h>

namespace saftbus {

	class Loop;
	class Source {
		// struct Impl; std::unique_ptr<Impl> d;
	friend class Loop;
	public:
		Source();
		virtual ~Source();
		virtual bool prepare(std::chrono::milliseconds &timeout_ms) = 0;
		virtual bool check() = 0;
		virtual bool dispatch() = 0;
	protected:
		void add_poll(struct pollfd &pfd);
		void remove_poll(struct pollfd &pfd);
		void destroy();
	private:
		Loop *loop;
		std::vector<struct pollfd*> pfds;
	};

	class Loop {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		Loop();
		~Loop();
		bool iteration(bool may_block);
		void run();
		bool quit();
		bool quit_in(std::chrono::milliseconds wait_ms);
		bool connect(std::unique_ptr<Source> source);

		template<typename T, typename... Args> // T must be subclass of Source
		bool connect(Args&&... args) {
			return connect(std::move(std::unique_ptr<T>(new T(std::forward<Args>(args)...))));
		}
		void remove(Source *s);
		void clear();
		static Loop &get_default();
	};

    /////////////////////////////////////
	// Define two useful Source types
    /////////////////////////////////////

    // call <slot> whenever <interval> amount of time has passed.
    // fist execution starts at <inteval>+<offset>
    // source is destroyed if <slot> returns false
	class TimeoutSource : public Source {
		// struct Impl; std::unique_ptr<Impl> d;
	public:
		TimeoutSource(std::function<bool(void)> slot, std::chrono::milliseconds interval, std::chrono::milliseconds offset = std::chrono::milliseconds(0));
		~TimeoutSource();
		bool prepare(std::chrono::milliseconds &timeout_ms) override;
		bool check() override;
		bool dispatch() override;
	private:
		std::function<bool(void)> slot;
		std::chrono::milliseconds interval;
		std::chrono::time_point<std::chrono::steady_clock> next_time;		
	};

	// call <slot> whenever <fd> fulfills <condition> (usually POLLIN or POLLOUT)
	// source is destroyed if POLLHP is seen on <fd>
	class IoSource : public Source {
		// struct Impl; std::unique_ptr<Impl> d;
	public:
		IoSource(std::function<bool(int, int)> slot, int fd, int condition);
		~IoSource();
		bool prepare(std::chrono::milliseconds &timeout_ms) override;
		bool check() override;
		bool dispatch() override;
	private:
		std::function<bool(int, int)> slot;
		struct pollfd pfd;
		// int id;
		// static int id_source;		
	};

}


#endif
