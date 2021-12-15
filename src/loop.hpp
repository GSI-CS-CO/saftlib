#ifndef MINI_SAFTLIB_LOOP_
#define MINI_SAFTLIB_LOOP_

#include <memory>
#include <chrono>
#include <sigc++/sigc++.h>

#include <poll.h>

namespace mini_saftlib {

	class Source {
		struct Impl; std::unique_ptr<Impl> d;
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
		void remove(Source *s);
		static Loop &get_default();
	};

    /////////////////////////////////////
	// Define two useful Source types
    /////////////////////////////////////

    // call <slot> whenever <interval> amount of time has passed.
    // fist execution starts at <inteval>+<offset>
    // source is destroyed if <slot> returns false
	class TimeoutSource : public Source {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		TimeoutSource(sigc::slot<bool> slot, std::chrono::milliseconds interval, std::chrono::milliseconds offset = std::chrono::milliseconds(0));
		~TimeoutSource();
		bool prepare(std::chrono::milliseconds &timeout_ms) override;
		bool check() override;
		bool dispatch() override;
	};

	// call <slot> whenever <fd> fulfills <condition> (usually POLLIN or POLLOUT)
	// source is destroyed if POLLHP is seen on <fd>
	class IoSource : public Source {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		IoSource(sigc::slot<bool, int, int> slot, int fd, int condition);
		~IoSource();
		bool prepare(std::chrono::milliseconds &timeout_ms) override;
		bool check() override;
		bool dispatch() override;
	};

}


#endif
