# gES (DONTREADME)
  A library that provides with an **Event System** designed for games specifically

## General Overview

``Hello World`` example:
```C++
#include <ges/dispatcher.hpp>
#include <string>
#include <iostream>

ges::dispatcher events;

struct MyEvent {
  std::string msg;
};

int main()
{
  events.listen([](const MyEvent& event) {
    std::cout << event.msg << '\n';
  });

  events.emit(MyEvent{"Press \'E\' to Play"});

  events.run();
}
```

gES is fully featured, supporting a general purpose **Event Bus** as well as the option to use **Event Batching**, **Immediate Dispatch** for a given event type.

Does not enforce you to sticking to a particular architecture, fits event-driven development, OOD, ECS or other DOD designs almost flawlessly.

It is fast as fuck, but not the fastest.
## How To Build 
gES utilizes CMake. You just clone the repo and run this:
```
TODO: build instructions, as though nobody knows how to use CMake and git
```
## Events, Event Handlers

Event types are simple ``struct``s, ``class``es, ``enum``s or any non built-in datatype. Events are not required to be POD types, that is they may or may not implement a user-defined destructor.

``ges::dispatcher`` is a registry for events. It manages events, listeners, buffers needed for context storage. You can have multiple **Event Dispatchers** which you can manage separately. 
```C++
void gameloop()
{
  while(true)
  {
    dispatcher.run();

    update_game(dt);
    render_game();
  }
}
``` 
Events don't need to be registered nor inherited from a base class. You only have to register an **Event Handler** by associating it with a given **Event Type** via ``listen``. 
```C++

  ges::dispatcher dispatcher;

  struct ChatMessage {
    std::string sender;
    std::string message;
  };

  static void on_chat_message(const ChatMessage& event)
  {
    println("[{0}]: {1}", event.sender, event.message);
  }
  
  int main()
  {
    dispatcher.listen(on_chat_message);

    while(true)
    {
      dispatcher.emit(ChatMessage{"LoveUrMoma2016", "gg"});
      dispatcher.run();
    }
  }

```
An Event Handler for a given event can have the following signatures.
```C++
void(const EventType&)
void(const EventType*) // C API, the pointer can be obtained instead
void(EventType) // Copy the event, also C compatible

/* pass in additional payload */
void(Instance*, const EventType&)
void(Instance*, const EventType*) // A C-style handler with payload
void(Instance*, EventType)

/* if a handler is a member function */
void(Instance::*)(const EventType&)
void(Instance::*)(const EventType*) 
void(Instance::*)(EventType)

/* your handler may not require any event data */
void()  
void(Instance*)
void(Instance::*)()
```
```C++
events.listen([](const EventType&){});
events.listen<EventType>([](const EventType&){}); // explicit template parameter
events.listen<EventType>([](){}); // template argument is required as 'listen' can't deduce Event Type to associate the handler
```
if you add payload, you should pass an instance pointer too, and make sure that handler lives no longer than the instance
```C++
  events.listen([](Instance*, const EventType&){}, &instance);
```
As of today, an Event Handler can be a free function, free function pointer, static member function, member function, non-capturing lambda, stateless functor, functor pointer

Callable objects such as ``std::function``, capturing lambdas, etc are not supported atm.

``listen`` function creates a delegate out of the handler. There are two interfaces to create a delegate. The first creates a fast delegate which recieves ``on_level_up`` as immediate constant, thus it gets statically linked or inlined. It can improve performance significantly if handler doesn't do much which results in noticeable overhead of additional indirect call.

```C++

struct LevelUpXP {
  entity_t entity;
  f64 amount;
};

void on_level_up(LevelUpXP event);

void init_game()
{
  events.listen<on_level_up>();
  // or 
  events.listen(on_level_up);
}

```
Note that member functions can be registered only by static binding. The reason is that member functions are special... Since you cannot legally type erase member function pointer.
```C++
void LevelingSystem::on_level_up(LevelUpXP event)
{
  auto entity = get_entity(event.entity);
  
  if(entity.xp == MAX_XP)
    return;

  entity.xp += event.amount;

  f64 next_lvl_up_xp = lookup_lvl_xp(entity.lvl + 1);

  if(entity.xp >= next_lvl_up_xp)
  {
    entity.lvl++;
  }
}

void LevelingSystem::init()
{
  events.listen<&LevelingSystem::on_level_up>(this);
}
```
Event Handlers are not akin to signals. Also their lifetime should be manually managed.
gES supports subscribing and unsubscribing.

``unlisten`` is the way to go:

```C++

void LevelingSystem::init()
{
  events.listen<&LevelingSystem::on_level_up>(this);
}

void LevelingSystem::shutdown()
{
  // TODO: write a descriptive comment here
  events.unlisten<&LevelingSystem::on_level_up>(this);
}

```
Note that registering/unregistering listeners within event handlers can result in Undefined Behaviour unless event handlers register or unregister themselves.

```C++

struct EnemyDied {
  uint64_t entity;
  uint64_t killed_by;
  std::string cause;
};

void AchievementSystem::unlock(Achievement achievo)
{
  switch(achievo) {
  case Achievement::FirstBlood: {
    std::cout << "Unlocked First Blood: Defeat your very first enemy\n";
  } break;
  case Achievement::Explorer: {
    std::cout << "Unlocked Explorer: Discover 100% of the map.\n";
  } break;
  case Achievement::WhomTheEnemyFears: {
    std::cout << "Unlocked Whom The Enemy Fears: Defeat a thousand enemies\n";
  } break;
  //...
  }
}

void AchievementSystem::unlock_kills(const EnemyDied& event)
{
  if(is_player(event.killed_by))
  {
    ++kill_count_;
  }
  
  switch(kill_count_) {
    case 1: {
      unlock(Achievement::FirstBlood);
    } return;
    case 1'000: {
      unlock(Achievement::WhomTheEnemyFears);
      // everything is unlocked, on need for handling it anymore
      events.unlisten<&AchievementSystem::unlock_kills>(this);
    } return;
  }
}

```
If your handler needs to be invoked once you can do it like so:
```C++
void hallway_jump_scare_once(PlayerEnteredSecretRoom)
{
  std::cout << "Boo\n";
  events.unlisten(hallway_jump_scare_once);
}
```
Call to ``trigger`` will dispatch handlers immediately blocking the loop.
```C++
struct FinalBossSpawned{};

...

if(player_entered_the_temple())
{
  events.trigger(FinalBossSpawned{});
}
```
## Event Bus
TODO
## Event Batching
TODO: