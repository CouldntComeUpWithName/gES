#include <ges/dispatcher.hpp>
#include <iostream>
#include <string>

struct test_event {
  std::string name = "test_event";
};

struct key_event { 
  int key; 
  bool pressed;
  bool repeated;
};

struct close_app { };

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
void test_event_handler(const test_event& )
{
  std::cout << "works! " << glob++ << '\n';
}

void another_handler(const test_event&)
{
  std::cout << "works as well!\n";
}

void show_message(const chat_message& event)
{
  std::cout << event.sender << ": " << event.msg << '\n';
}

void notify_message(const chat_message& event)
{

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

struct stateless {
  void operator()(test_event)
  {
    std::cout << "stateless call operator\n";
  }
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
  ges::dispatcher dispatcher;
  message_notifier notifications;
  
  dispatcher.listen<test_event, test_event_handler>();
  dispatcher.listen<test_event>(another_handler);

  functor functor(dispatcher);
  
  dispatcher.listen<chat_message, show_message>();
  dispatcher.listen<chat_message, notify_message>();
  
  dispatcher.listen<chat_message, &message_notifier::notify>(&notifications);
  
  dispatcher.listen<key_event, on_key_event>();



  dispatcher.batch<chat_message>("Tom", "Hello");
  dispatcher.batch<key_event>(1, true, true);
  
  dispatcher.batch<chat_message>("Liam", "How are you doing guys?");

  dispatcher.run();
  
  if(dispatcher.erase<test_event>(another_handler))
  {
    std::cout << "another_handler is erased\n";
  }

  if (dispatcher.erase<chat_message, &message_notifier::notify>(&notifications))
  {
    std::cout << "message_notifier::notify is erased\n";
  }

  if (dispatcher.erase<test_event>(stateless{}))
  {
    std::cout << "stateless is erased\n";
  }

  if (dispatcher.erase<test_event>(&functor))
  {
    std::cout << "functor is erased\n";
  }

  for (int i = 0; i < 10; i++)
  {
    dispatcher.batch<test_event>();
  }

  dispatcher.run();
}
