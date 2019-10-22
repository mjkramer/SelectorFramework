#include <iostream>

#include "../core/Kernel.cc"
#include "../core/TreeWriter.cc"
#include "../core/SyncReader.cc"

struct MyData : public TreeBase {
  int x;
  float y;
  std::array<double, 4> zs;

  unsigned short bufsize;
  std::array<float, 256> buf;

  void initBranches() override;
};

void MyData::initBranches()
{
  BR(x);
  BR_NAMED(y, "y");
  BR(zs);
  BR(bufsize);
  BR_VARLEN(buf, bufsize);
}

void test_write()
{
  Pipeline p;

  p.makeOutFile("out_test.root");

  TreeWriter<MyData> w("foo_AD1", "All the pitiful foos for AD1");
  w.connect(p);

  w.data.x = 3;
  w.data.y = 9;
  w.data.zs = {4, 3, 2, 6};
  w.data.bufsize = 2;
  w.data.buf = {77, 66};
  w.fill();

  w.data.x = 99;
  w.data.y = 3.14159;
  w.data.zs = {1, 2, 3, 4};
  w.data.bufsize = 1;
  w.data.buf = {73};
  w.fill();
}

Algorithm::Status readerTestAlg(const MyData& data)
{
  std::cout << data.x << " " << data.y << std::endl;
  std::cout << data.zs[0] << " " << data.zs[1] << " " << data.zs[2] << " " << data.zs[3] << std::endl;
  std::cout << data.bufsize << std::endl;
  for (size_t i = 0; i < data.bufsize; ++i)
    std::cout << data.buf[i] << " ";
  std::cout << std::endl << std::endl;

  return Algorithm::Status::Continue;
}

using ReaderTestAlg = PureAlg<SyncReader<MyData>, readerTestAlg>;

void test_read()
{
  Pipeline p;

  // have to put the list of tree names in a temporary variable because C++
  // isn't smart enough to do the deduction/forwarding when we plop {"foo_AD1"}
  // directly into the call to makeAlg (unless we explicitly cast it as an
  // init_list)
  auto treeNames = {"foo_AD1"}; // std::initializer_list<const char*>
  p.makeAlg<SyncReader<MyData>>(treeNames);
  p.makeAlg<ReaderTestAlg>();

  p.process({"out_test.root"});
}
