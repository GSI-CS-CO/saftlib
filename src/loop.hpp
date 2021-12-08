#ifndef MINI_SAFTLIB_LOOP_
#define MINI_SAFTLIB_LOOP_

#include <memory>
#include <chrono>
#include <sigc++/sigc++.h>

#include <poll.h>

namespace mini_saftlib {

	class Loop {
	public:
		Loop();
		~Loop();
		bool iteration(bool may_block);
		void run();
		bool connect_io(sigc::slot<bool, int, int> slot, int fd, int condition);
		bool connect_timeout(sigc::slot<bool> slot, std::chrono::milliseconds interval, std::chrono::milliseconds offset = std::chrono::milliseconds(0));
	private:
		struct Impl;
		std::unique_ptr<Impl> d;
	};

	class Source {
	public:
		Source();
		virtual ~Source();
		virtual bool prepare(std::chrono::milliseconds &timeout_ms) = 0;
		virtual bool check() = 0;
		virtual bool dispatch() = 0;
	protected:
		void add_poll(struct pollfd &pfd);
		void remove_poll(struct pollfd &pfd);
	private:
		struct Impl;
		std::unique_ptr<Impl> d;

		const std::vector<struct pollfd*> &get_pfds() const ;
		friend class Loop;
	};

	class TimeoutSource : public Source {
	public:
		TimeoutSource(sigc::slot<bool> slot, std::chrono::milliseconds interval, std::chrono::milliseconds offset);
		~TimeoutSource();
		bool prepare(std::chrono::milliseconds &timeout_ms) override;
		bool check() override;
		bool dispatch() override;
	private:
		struct Impl;
		std::unique_ptr<Impl> d;
	};

	class IoSource : public Source {
	public:
		IoSource(sigc::slot<bool, int, int> slot, int fd, int condition);
		~IoSource();
		bool prepare(std::chrono::milliseconds &timeout_ms) override;
		bool check() override;
		bool dispatch() override;
	private:
		struct Impl;
		std::unique_ptr<Impl> d;
	};

}


#endif
