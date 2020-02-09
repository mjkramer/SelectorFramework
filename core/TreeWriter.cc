#include "TreeWriter.hh"

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
