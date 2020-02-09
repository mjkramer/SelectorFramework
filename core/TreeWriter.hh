#pragma once

#include "BaseIO.hh"
#include "Kernel.hh"

template <class TreeT>          // TreeT <: TreeBase
class TreeWriter {
public:
  TreeWriter(const char* path, const char* title = nullptr);
  TreeWriter(TreeWriter<TreeT>&& other);
  TreeWriter(const TreeWriter<TreeT>& other) = delete;
  ~TreeWriter();

  void connect(Pipeline&p, const char* outFileName = Pipeline::DefaultFile);
  void fill();
  TTree* tree();

  TreeT data;

private:
  BranchManager mgr;
};


template <class TreeT>
inline
TTree* TreeWriter<TreeT>::tree()
{
  return mgr.tree;
}

template <class TreeT>
inline
void TreeWriter<TreeT>::fill()
{
  mgr.tree->Fill();
}
