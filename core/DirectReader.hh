#pragma once

#include "BaseIO.hh"

#include <TFile.h>
#include <TTree.h>

template <class TreeT>          // TreeT <: TreeBase
class DirectReader {
public:
  DirectReader() {};
  DirectReader(TFile* file, const char* treeName);
  void init(TFile* file, const char* treeName);
  size_t size();
  void loadEntry(size_t entry);
  const TreeT& at(size_t entry);

  TreeT data;

protected:
  BranchManager mgr {BranchManager::IOMode::IN};
};

template <class TreeT>
DirectReader<TreeT>::DirectReader(TFile* file, const char* treeName)
{
  init(file, treeName);
}

template <class TreeT>
void DirectReader<TreeT>::init(TFile* file, const char* treeName)
{
  auto tree = dynamic_cast<TTree*>(file->Get(treeName));
  tree->SetMakeClass(true);
  tree->SetBranchStatus("*", false);
  mgr.tree = tree;
  data.setManager(&mgr);
  data.initBranches();
}

template <class TreeT>
size_t DirectReader<TreeT>::size()
{
  return mgr.tree->GetEntries();
}

template <class TreeT>
void DirectReader<TreeT>::loadEntry(size_t entry)
{
  mgr.tree->GetEntry(entry);
}

template <class TreeT>
const TreeT& DirectReader<TreeT>::at(size_t entry)
{
  loadEntry(entry);
  return data;
}
