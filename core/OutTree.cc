#pragma once

#include <TTree.h>

#include "Kernel.cc"

class OutTree {
public:
  OutTree(const char* path, const char* title = nullptr);
  ~OutTree();
  void connect(Pipeline& p, const char* outFileName = Pipeline::DefaultFile);
  void fill();
  TTree* tree() { return t; }

protected:
  virtual void initBranches() {};

  TTree* t;
};

OutTree::OutTree(const char* path, const char* title)
{
  t = new TTree(path, title ? title : path);
}

OutTree::~OutTree()
{
  t->Write();
}

void OutTree::connect(Pipeline& p, const char* outFileName)
{
  TFile* f = p.getOutFile(outFileName);
  t->SetDirectory(f);           // TODO Support subdirectories
  initBranches();
}

void OutTree::fill()
{
  t->Fill();
}

#define OB(branch, typecode) \
  t->Branch(#branch, &branch, #branch "/" typecode)
