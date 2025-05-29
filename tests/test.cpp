#include <ges/dispatcher.hpp>
#include <iostream>
#include <string>

ges::dispatcher dispatcher;

struct dummy_event { };

struct test_event {
  std::string name = "test event";
};

struct key_event { 
  int key; 
  bool pressed;
  bool repeated;
};

struct close_event { };

struct chat_message {
  std::string sender;
  std::string msg;
};

struct trivial_t {
  int i = 0;
};

struct non_trivial {

  int i = 0;
  ~non_trivial() {
    i++;
    std::cout << "I'm called " << i << " times\n";
  }

};

void on_key_event(const key_event& event)
{
  std::cout << "key pressed: " << event.key << " " << event.pressed 
  << " " << event.repeated << '\n';
}

int glob = 0;
void test_event_handler(const test_event&)
{
  std::cout << "works! " << glob++ << '\n';
}

void on_test(const test_event&)
{
  std::cout << "works as well!\n";
}

void handle_test(const test_event&)
{
  std::cout << "handing some stuff...\n";
}

void show_message(const chat_message& event)
{
  std::cout << event.sender << ": " << event.msg << '\n';
}

void notify_message(const chat_message& event)
{

}

void once(const test_event& test)
{
  std::cout << "once\n";
  if(test.name == "test event")
  {

  }
  
}

struct message_notifier {
  void notify(const chat_message& event)
  {
    std::cout << "---" << std::endl;
    std::cout << "Notification: you got a message from " << event.sender << '\n';
    std::cout << "Incoming messages: " << ++incoming << std::endl;
    std::cout << "---" << std::endl;
  }
private:
  uint32_t incoming = 0;
};

struct functor {
  
  functor(ges::dispatcher& dispatcher)
    : dispatcher_{dispatcher}
  {
    dispatcher_.listen<test_event>(this);
  }

  auto operator()(test_event)
  {
    std::cout << name << " call operator\n";
  }

  ~functor()
  {
    if (dispatcher_.erase<test_event>(this))
    {
      std::cout << "functor is erased\n";
    }
  }

  std::string name = "functor";
  ges::dispatcher& dispatcher_;
};

int main()
{
  message_notifier notifications;

  dispatcher.listen<test_event, test_event_handler>();
  dispatcher.listen<test_event, on_test>();
  dispatcher.listen<test_event, once>();
  dispatcher.listen<test_event, handle_test>();

  functor functor(dispatcher);
  
  dispatcher
    .listen<chat_message, show_message>()
    .listen<chat_message, notify_message>()
    .listen<chat_message, &message_notifier::notify>(&notifications);

  dispatcher.listen<key_event, on_key_event>();
  for (int i = 0; i < 10; i++)
  {
    dispatcher.batch<test_event>();
    dispatcher.enqueue<test_event>();
  }

  dispatcher.enqueue(chat_message{ "Tom", "Hello" });
  
  dispatcher.enqueue(chat_message{
    .sender = "Tom2",
    .msg = "Hello2"
  });

  dispatcher.batch<key_event>(1, true, true);

  dispatcher.batch<chat_message>("Liam", "How are you doing guys?");

  if (dispatcher.contains<test_event>())
    std::cout << "there are test_event handlers\n";

  if (dispatcher.has<test_event>())
    std::cout << "test_event is registered, but handlers be not bound\n";

  dispatcher.run();

  if (dispatcher.erase<chat_message, &message_notifier::notify>(&notifications))
  {
    std::cout << "message_notifier::notify is erased\n";
  }

  dispatcher.clear<test_event>();

  if (!dispatcher.contains<test_event>())
    std::cout << "test_event handlers are empty\n";

  for (int i = 0; i < 10; i++)
  {
    dispatcher.batch<test_event>();
  }

  dispatcher.run();
}
