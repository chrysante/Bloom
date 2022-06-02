#pragma once

#include <utl/messenger.hpp>
#include <utl/functional.hpp>

namespace bloom {

	enum class DispatchToken {
		nextFrame, now
	};
	
	namespace dispatchTokens {
		constexpr DispatchToken nextFrame = DispatchToken::nextFrame;
		constexpr DispatchToken now = DispatchToken::now;
	};
	
	/// MARK: Emitter
	class Emitter: private utl::emitter<utl::buffered_messenger> {
		friend class Messenger;
		using MyBase = utl::emitter<utl::buffered_messenger>;
		using MyBase::MyBase;
		
	public:
		void dispatch(DispatchToken token, std::any message) {
			using namespace dispatchTokens;
			switch (token) {
				case nextFrame:
					MyBase::send(std::move(message));
					break;
				case now:
					MyBase::send_now(std::move(message));
					break;
			}
		}
		void assignEmitter(Emitter emitter) { *this = std::move(emitter); }
	};
	
	/// MARK: Reciever
	class Reciever: private utl::reciever<utl::buffered_messenger> {
	public:
		friend class Messenger;
		using MyBase = utl::reciever<utl::buffered_messenger>;
		using MyBase::MyBase;
		
		void assignReciever(Reciever reciever) { *this = std::move(reciever); }
		
	public:
		using MyBase::listen;
	};
	
	/// MARK: MessageSystem
	class MessageSystem {
	public:
		MessageSystem(): m(std::make_unique<utl::buffered_messenger>()) {}
		
		Emitter makeEmitter() { return Emitter(m); }
		Reciever makeReciever() { return Reciever(m); }
		
		void flush() { m->flush(); }
		
	private:
		std::shared_ptr<utl::buffered_messenger> m;
	};
	
}

