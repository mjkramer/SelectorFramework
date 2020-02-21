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

template <class TreeT>
TreeWriter<TreeT>::TreeWriter(const char* path, const char* title) :
  mgr(BranchManager::IOMode::OUT)
{
  mgr.tree = new TTree(path, title ? title : path);
}

template <class TreeT>
TreeWriter<TreeT>::TreeWriter(TreeWriter<TreeT>&& other)
{
  mgr = other.mgr;
  other.mgr.tree = nullptr;
}

template <class TreeT>
void TreeWriter<TreeT>::connect(Pipeline& p, const char* outFileName)
{
  TFile* f = p.getOutFile(outFileName);
  mgr.tree->SetDirectory(f);           // TODO Support subdirectories

  data.setManager(&mgr);
  data.initBranches();
}

template <class TreeT>
TreeWriter<TreeT>::~TreeWriter()
{
  if (mgr.tree) {
    mgr.tree->GetDirectory()->cd();
    mgr.tree->Write();
  }
}
