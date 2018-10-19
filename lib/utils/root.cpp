/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2018-10
 */

#include "root.h"

#include <Compression.h>

#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Root)

static void closeTFileRead(TFile* f)
{
  if (!f) {
    return;
  }
  if (f->IsZombie()) {
    ERROR("could not close '", f->GetName(), "' because it eats brains");
  } else if (!f->IsOpen()) {
    ERROR("could not close '", f->GetName(), "' because it is not open");
  } else {
    f->Close();
  }
  // delete is independent of open/close status
  delete f;
}

static void closeTFileWrite(TFile* f)
{
  if (!f) {
    return;
  }
  if (f->IsZombie()) {
    ERROR("could not close '", f->GetName(), "' because it eats brains");
  } else if (!f->IsOpen()) {
    ERROR("could not close '", f->GetName(), "' because it is not open");
  } else {
    // ensure everything is written to disk before closing
    auto bytes = f->Write(nullptr, TFile::kOverwrite);
    VERBOSE("wrote ", bytes, " bytes to '", f->GetName(), "'");
    f->Close();
  }
  // delete is independent of open/close status
  delete f;
}

Utils::RootFilePtr Utils::openRootRead(const std::string& path)
{
  RootFilePtr f(TFile::Open(path.c_str(), "READ"), &closeTFileRead);
  if (!f) {
    throw std::runtime_error("Could not open '" + path + "' to read");
  }
  if (!f->IsOpen()) {
    throw std::runtime_error("'" + path + "' is not open");
  }
  return f;
}

Utils::RootFilePtr Utils::openRootWrite(const std::string& path)
{
  // always use better, non-standard compression
  RootFilePtr f(TFile::Open(path.c_str(), "RECREATE", "",
                            ROOT::CompressionSettings(ROOT::kLZMA, 1)),
                &closeTFileWrite);
  if (!f) {
    throw std::runtime_error("Could not open '" + path + "' to write");
  }
  if (!f->IsOpen()) {
    throw std::runtime_error("'" + path + "' is not open");
  }
  return f;
}

TDirectory* Utils::makeDir(TDirectory* parent, const std::string& path)
{
  assert(parent && "Parent directory must be non-NULL");

  TDirectory* dir = parent->GetDirectory(path.c_str());
  if (!dir) {
    // the return value of this is useless:
    // NULL means both the directory exists and everything is ok, or a failure.
    // not-NULL returns only the first subdirectory if path defines a hierachy,
    // but we want the final directory that was created.
    dir = parent->mkdir(path.c_str());
    dir = parent->GetDirectory(path.c_str());
  }
  if (!dir) {
    throw std::runtime_error("Could not create ROOT directory '" + path + '\'');
  }
  return dir;
}

TH1D* Utils::makeH1(TDirectory* dir, const std::string& name, HistAxis axis)
{
  assert(dir && "Directory must be non-NULL");

  TH1D* h = new TH1D(name.c_str(), "", axis.bins, axis.limit0, axis.limit1);
  h->SetXTitle(axis.label.c_str());
  h->SetDirectory(dir);
  return h;
}

TH2D* Utils::makeH2(TDirectory* dir,
                    const std::string& name,
                    HistAxis axis0,
                    HistAxis axis1)
{
  assert(dir && "Directory must be non-NULL");

  TH2D* h = new TH2D(name.c_str(), "", axis0.bins, axis0.limit0, axis0.limit1,
                     axis1.bins, axis1.limit0, axis1.limit1);
  h->SetXTitle(axis0.label.c_str());
  h->SetYTitle(axis1.label.c_str());
  h->SetDirectory(dir);
  return h;
}

TH1D* Utils::makeTransientH1(HistAxis axis)
{
  // try to generate a (unique) name. not sure if needed
  std::string name;
  name += axis.label;
  name += std::to_string(axis.limit0);
  name += std::to_string(axis.limit1);
  name += std::to_string(axis.bins);

  TH1D* h = new TH1D(name.c_str(), "", axis.bins, axis.limit0, axis.limit1);
  h->SetXTitle(axis.label.c_str());
  h->SetDirectory(nullptr);
  return h;
}

TH2D* Utils::makeTransientH2(HistAxis axis0, HistAxis axis1)
{
  // try to generate a (unique) name. not sure if needed
  std::string name;
  name += axis0.label;
  name += std::to_string(axis0.limit0);
  name += std::to_string(axis0.limit1);
  name += std::to_string(axis0.bins);
  name += axis1.label;
  name += std::to_string(axis1.limit0);
  name += std::to_string(axis1.limit1);
  name += std::to_string(axis1.bins);

  TH2D* h = new TH2D("", "", axis0.bins, axis0.limit0, axis0.limit1, axis1.bins,
                     axis1.limit0, axis1.limit1);
  h->SetXTitle(axis0.label.c_str());
  h->SetYTitle(axis1.label.c_str());
  h->SetDirectory(nullptr);
  return h;
}

void Utils::fillDist(const TH2D* values, TH1D* dist)
{
  // ensure all values are binned
  dist->SetBins(dist->GetNbinsX(), values->GetMinimum(),
                std::nextafter(values->GetMaximum(), values->GetMaximum() + 1));
  for (int icol = 1; icol <= values->GetNbinsX(); ++icol) {
    for (int irow = 1; irow <= values->GetNbinsY(); ++irow) {
      auto value = values->GetBinContent(icol, irow);
      if (std::isfinite(value)) {
        dist->Fill(value);
      }
    }
  }
}

std::pair<double, double> Utils::getRestrictedMean(const TH1D* histo,
                                                   const int offset)
{
  // TODO uncomment it after verifying with the updated alignment by Moritz
  // assert(offset >= 0);
  TH1D h;
  h.SetDirectory(nullptr);
  histo->Copy(h);
  int maxBin = h.GetMaximumBin();
  if (offset >= 0) {
    h.GetXaxis()->SetRange(maxBin - offset, maxBin + offset);
  }
  double mean = h.GetMean();
  double var = h.GetMeanError() * h.GetMeanError();
  return std::make_pair(mean, var);
}
