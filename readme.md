# thread_pool
a general purpose C++ 17+ header-only thread pool implementation

## usage
see thread_pool_test.cc

basic example:
```
auto tp = thread_ext::thread_pool(3);
uint8_t val = 5;
auto task = [val] { return val; };

auto fut1 = tp.submit<uint8_t>(task);
// note: each schedule call reserves a dedicated thread
// from within the thread pool to block task execution
// until the input delay ms time has expired
// avoid task queue backpressure by keeping schedule
// calls bounded and delay ms limited
auto fut2 = tp.schedule<void>([] {}, 1000);

uint8_t n1 = fut1->get();
fut2->get();

tp.shutdown();
```

## platforms
the following platforms were tested

* Visual Studio 2019+
* clang 11.0.0+
