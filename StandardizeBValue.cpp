/*-
 * Copyright (c) 2018 Nathan Lay (enslay@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Nathan Lay
 * Imaging Biomarkers and Computer-Aided Diagnosis Laboratory
 * National Institutes of Health
 * March 2017
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdlib>
#include <cstdint>
#include <cctype>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>
#include "Common.h"
#include "bsdgetopt.h"
#include "strcasestr.h"

// ITK stuff
#include "itkGDCMImageIO.h"
#include "itkMetaDataDictionary.h"
#include "itkMetaDataObject.h"
#include "itkRGBPixel.h"
#include "itkRGBAPixel.h"

#include "gdcmBase64.h"
#include "gdcmCSAHeader.h"
#include "gdcmCSAElement.h"
 
void Usage(const char *p_cArg0) {
  std::cerr << p_cArg0 << " [-hr] path|filePattern [path2|filePattern2 ...]" << std::endl;
  std::cerr << "\nOptions:" << std::endl;
  std::cerr << "-h -- This help message." << std::endl;
  std::cerr << "-r -- Recursively search folders." << std::endl;
  exit(1);
}

bool IsHexDigit(char c);
bool ParseITKTag(const std::string &strKey, uint16_t &ui16Group, uint16_t &ui16Element);

template<typename ValueType>
bool ExposeCSAMetaData(gdcm::CSAHeader &clHeader, const char *p_cKey, ValueType &value);

template<>
bool ExposeCSAMetaData<std::string>(gdcm::CSAHeader &clHeader, const char *p_cKey, std::string &strValue);

bool GetCSAHeaderFromElement(const itk::MetaDataDictionary &clDicomTags, const std::string &strKey, gdcm::CSAHeader &clCSAHeader);

std::string ComputeDiffusionBValue(const itk::MetaDataDictionary &clDicomTags);
std::string ComputeDiffusionBValueSiemens(const itk::MetaDataDictionary &clDicomTags);
std::string ComputeDiffusionBValueGE(const itk::MetaDataDictionary &clDicomTags);
std::string ComputeDiffusionBValueProstateX(const itk::MetaDataDictionary &clDicomTags); // Same as Skyra and Verio
std::string ComputeDiffusionBValuePhilips(const itk::MetaDataDictionary &clDicomTags);

bool StandardizeBValue(const std::string &strFileName);

template<typename PixelType>
bool StandardizeBValueHelper(const std::string &strFileName);

int main(int argc, char **argv) {
  const char * const p_cArg0 = argv[0];
  
  bool bRecursive = false;
  
  int c = 0;
  while ((c = getopt(argc, argv, "hr")) != -1) {
    switch (c) {
    case 'h':
      Usage(p_cArg0);
      break;
    case 'r':
      bRecursive = true;
      break;
    case '?':
    default:
      Usage(p_cArg0);
    }
  }
  
  argc -= optind;
  argv += optind;
  
  if (argc <= 0)
    Usage(p_cArg0);
  
  std::vector<std::string> vFiles;

  for (int i = 0; i < argc; ++i) {
    const char * const p_cFile = argv[i];

    if (strpbrk(p_cFile, "?*") != nullptr) {
      // DOS wildcard pattern
      FindFiles(DirName(p_cFile).c_str(), p_cFile, vFiles, bRecursive);
    }
    else if (IsFolder(p_cFile)) {
      // Directory
      FindFiles(p_cFile, "*", vFiles, bRecursive);
    }
    else {
      // Individual file
      vFiles.push_back(p_cFile);
    }
  }

  for (const std::string &strFile : vFiles) {
    std::cout << "Info: Processing '" << strFile << "' ..." << std::endl;
    StandardizeBValue(strFile);
  }

  std::cout << "Done." << std::endl;

  return 0;
}

bool IsHexDigit(char c) {
  if (std::isdigit(c))
    return true;

  switch (std::tolower(c)) {
  case 'a':
  case 'b':
  case 'c':
  case 'd':
  case 'e':
  case 'f':
    return true;
  }

  return false;
}

bool ParseITKTag(const std::string &strKey, uint16_t &ui16Group, uint16_t &ui16Element) {
  if (strKey.empty() || !IsHexDigit(strKey[0]))
    return false;

  ui16Group = ui16Element = 0;

  char *p = nullptr;
  unsigned long ulTmp = strtoul(strKey.c_str(), &p, 16);

  if (*p != '|' || *(p+1) == '\0' || ulTmp > std::numeric_limits<uint16_t>::max())
    return false;

  ui16Group = (uint16_t)ulTmp;

  ulTmp = strtoul(p+1, &p, 16);

  if (*p != '\0' || ulTmp > std::numeric_limits<uint16_t>::max())
    return false;

  ui16Element = (uint16_t)ulTmp;

  return true;
}

template<typename ValueType>
bool ExposeCSAMetaData(gdcm::CSAHeader &clHeader, const char *p_cKey, ValueType &value) {
  if (!clHeader.FindCSAElementByName(p_cKey))
    return false;

  const gdcm::CSAElement &clElement = clHeader.GetCSAElementByName(p_cKey);
  const gdcm::ByteValue * const p_clByteValue = clElement.GetByteValue();

  if (p_clByteValue == nullptr || p_clByteValue->GetLength() != sizeof(ValueType))
    return false;

  return p_clByteValue->GetBuffer((char *)&value, sizeof(ValueType));
}

template<>
bool ExposeCSAMetaData<std::string>(gdcm::CSAHeader &clHeader, const char *p_cKey, std::string &strValue) {
  if (!clHeader.FindCSAElementByName(p_cKey))
    return false;

  const gdcm::CSAElement &clElement = clHeader.GetCSAElementByName(p_cKey);
  const gdcm::ByteValue * const p_clByteValue = clElement.GetByteValue();

  if (p_clByteValue == nullptr)
    return false;

  std::vector<char> vBuffer(p_clByteValue->GetLength());
  if (!p_clByteValue->GetBuffer(&vBuffer[0], vBuffer.size()))
    return false;

  strValue.assign(vBuffer.begin(), vBuffer.end());

  return true;
}


bool GetCSAHeaderFromElement(const itk::MetaDataDictionary &clDicomTags, const std::string &strKey, gdcm::CSAHeader &clCSAHeader) {
  uint16_t ui16Group = 0, ui16Element = 0;

  clCSAHeader = gdcm::CSAHeader();

  if (!ParseITKTag(strKey, ui16Group, ui16Element))
    return false;

  std::string strValue;

  if (!itk::ExposeMetaData<std::string>(clDicomTags, strKey, strValue))
    return false;

  const int iDecodeLength = gdcm::Base64::GetDecodeLength(strValue.c_str(), (int)strValue.size());

  if (iDecodeLength <= 0)
    return false;

  std::vector<char> vBuffer(iDecodeLength);

  if (gdcm::Base64::Decode(&vBuffer[0], vBuffer.size(), strValue.c_str(), strValue.size()) == 0)
    return false;

  gdcm::DataElement clDataElement;
  clDataElement.SetTag(gdcm::Tag(ui16Group, ui16Element));
  clDataElement.SetByteValue(&vBuffer[0], vBuffer.size());

  return clCSAHeader.LoadFromDataElement(clDataElement);
}

std::string ComputeDiffusionBValue(const itk::MetaDataDictionary &clDicomTags) {
  std::string strBValue;

  if (itk::ExposeMetaData(clDicomTags, "0018|9087", strBValue)) {
    Trim(strBValue);
    return strBValue;
  }

  std::string strPatientName;
  std::string strPatientId;
  std::string strManufacturer;

  itk::ExposeMetaData(clDicomTags, "0010|0010", strPatientName);
  itk::ExposeMetaData(clDicomTags, "0010|0020", strPatientId);

  if (strcasestr(strPatientName.c_str(), "prostatex") != nullptr || strcasestr(strPatientId.c_str(), "prostatex") != nullptr)
    return ComputeDiffusionBValueProstateX(clDicomTags);

  if (!itk::ExposeMetaData(clDicomTags, "0008|0070", strManufacturer)) {
    std::cerr << "Error: Could not determine manufacturer." << std::endl;
    return std::string();
  }

  if (strcasestr(strManufacturer.c_str(), "siemens") != nullptr)
    return ComputeDiffusionBValueSiemens(clDicomTags);
  else if (strcasestr(strManufacturer.c_str(), "ge") != nullptr)
    return ComputeDiffusionBValueGE(clDicomTags);
  else if (strcasestr(strManufacturer.c_str(), "philips") != nullptr)
    return ComputeDiffusionBValuePhilips(clDicomTags);

  return std::string();
}

std::string ComputeDiffusionBValueSiemens(const itk::MetaDataDictionary &clDicomTags) {
  std::string strModel;

  if (itk::ExposeMetaData(clDicomTags, "0008|1090", strModel)) {
    if ((strcasestr(strModel.c_str(), "skyra") != nullptr || strcasestr(strModel.c_str(), "verio") != nullptr)) {
      std::string strTmp = ComputeDiffusionBValueProstateX(clDicomTags);

      if (strTmp.size() > 0)
        return strTmp;
    }
  }

  gdcm::CSAHeader clCSAHeader;

  if (!GetCSAHeaderFromElement(clDicomTags, "0029|1010", clCSAHeader)) // Nothing to do
    return std::string();

  std::string strTmp;

  if (ExposeCSAMetaData(clCSAHeader, "B_value", strTmp))
    return strTmp;

  return std::string();
}

std::string ComputeDiffusionBValueGE(const itk::MetaDataDictionary &clDicomTags) {
  std::string strValue;
  if (!itk::ExposeMetaData(clDicomTags, "0043|1039", strValue))
    return std::string(); // Nothing to do

  size_t p = strValue.find('\\');
  if (p == std::string::npos)
    return std::string(); // Not sure what to do

  strValue.erase(p);

  std::stringstream valueStream;
  valueStream.str(strValue);

  double dValue = 0.0;

  if (!(valueStream >> dValue) || dValue < 0.0) // Bogus value
    return std::string();

  // Something is screwed up here ... let's try to remove the largest significant digit
  if (dValue > 3000.0) {
    p = strValue.find_first_not_of(" \t0");

    strValue.erase(strValue.begin(), strValue.begin()+p+1);

    valueStream.clear();
    valueStream.str(strValue);

    if (!(valueStream >> dValue) || dValue < 0.0 || dValue > 3000.0)
      return std::string();
  }

  return std::to_string((long long)dValue);
}

std::string ComputeDiffusionBValueProstateX(const itk::MetaDataDictionary &clDicomTags) {
  std::string strSequenceName;
  if (!itk::ExposeMetaData(clDicomTags, "0018|0024", strSequenceName)) {
    std::cerr << "Error: Could not extract sequence name (0018,0024)." << std::endl;
    return std::string();
  }

  Trim(strSequenceName);

  if (strSequenceName.empty()) {
    std::cerr << "Error: Empty sequence name (0018,0024)." << std::endl;
    return std::string();
  }

  std::stringstream valueStream;

  unsigned int uiBValue = 0;

  size_t i = 0, j = 0;
  while (i < strSequenceName.size()) {
    i = strSequenceName.find('b', i); 

    if (i == std::string::npos || ++i >= strSequenceName.size())
      break;

    j = strSequenceName.find_first_not_of("0123456789", i); 

    // Should end with a 't' or a '\0'
    if (j == std::string::npos)
      j = strSequenceName.size();
    else if (strSequenceName[j] != 't')
      break;

    if (j > i) {
      std::string strBValue = strSequenceName.substr(i, j-i);
      valueStream.clear();
      valueStream.str(strBValue);

      uiBValue = 0;

      if (valueStream >> uiBValue) {
        if (uiBValue < 4000)
          return strBValue;
        else
          std::cerr << "Error: B-value of " << uiBValue << " seems bogus. Continuing to parse." << std::endl;
      }   
    }   

    i = j;
  }

  std::cerr << "Error: Could not parse sequence name '" << strSequenceName << "'." << std::endl;

  return std::string();
}

std::string ComputeDiffusionBValuePhilips(const itk::MetaDataDictionary &clDicomTags) {
  std::string strBValue;

  if (!itk::ExposeMetaData(clDicomTags, "2001|1003", strBValue))
    return std::string();

  return strBValue;
}

bool StandardizeBValue(const std::string &strFileName) {
  typedef itk::GDCMImageIO ImageIOType;

  ImageIOType::Pointer p_clImageIO = ImageIOType::New();

  if (!p_clImageIO->CanReadFile(strFileName.c_str())) {
    std::cerr << "Error: Could not read '" << strFileName << "' (not a DICOM?)." << std::endl;
    return false;
  }

  p_clImageIO->SetFileName(strFileName);
  p_clImageIO->KeepOriginalUIDOn();
  p_clImageIO->LoadPrivateTagsOn();

  try {
    p_clImageIO->ReadImageInformation();
  }
  catch (itk::ExceptionObject &e) {
    std::cerr << "Error: " << e << std::endl;
    return false;
  }

  const itk::MetaDataDictionary &clDicomTags = p_clImageIO->GetMetaDataDictionary();

  std::string strModality;
  if (!itk::ExposeMetaData(clDicomTags, "0008|0060", strModality)) {
    std::cerr << "Error: Could not determine image modality." << std::endl;
    return false;
  }

  Trim(strModality);

  if (strModality != "MR") {
    std::cerr << "Error: Incorrect imaging modality (" << strModality << " != MR)." << std::endl;
    return false;
  }

  std::string strBValue;
  if (itk::ExposeMetaData(clDicomTags, "0018|9087", strBValue)) {
    Trim(strBValue);
    std::cerr << "Error: Diffusion b-value is already standardized (b = " << strBValue << ")." << std::endl;
    return true;
  }

  // Support possibly weird images?
  switch (p_clImageIO->GetPixelType()) {
  case ImageIOType::SCALAR:
    switch (p_clImageIO->GetInternalComponentType()) {
    case ImageIOType::UCHAR:
      return StandardizeBValueHelper<unsigned char>(strFileName);
    case ImageIOType::CHAR:
      return StandardizeBValueHelper<char>(strFileName);
    case ImageIOType::USHORT:
      return StandardizeBValueHelper<unsigned short>(strFileName);
    case ImageIOType::SHORT:
      return StandardizeBValueHelper<short>(strFileName);
    case ImageIOType::UINT:
      return StandardizeBValueHelper<unsigned int>(strFileName);
    case ImageIOType::INT:
      return StandardizeBValueHelper<int>(strFileName);
    case ImageIOType::FLOAT:
      return StandardizeBValueHelper<float>(strFileName);
    case ImageIOType::DOUBLE:
      return StandardizeBValueHelper<double>(strFileName);
    default:
      std::cerr << "Error: Unknown scalar component type." << std::endl;
      return false;
    }
    break;
  case ImageIOType::RGB:
    switch (p_clImageIO->GetInternalComponentType()) {
    case ImageIOType::UCHAR:
      return StandardizeBValueHelper<itk::RGBPixel<unsigned char>>(strFileName);
    default:
      std::cerr << "Error: Unknown RGB component type." << std::endl;
      return false;
    }
    break;
  case ImageIOType::RGBA:
    switch (p_clImageIO->GetInternalComponentType()) {
    case ImageIOType::UCHAR:
      return StandardizeBValueHelper<itk::RGBAPixel<unsigned char>>(strFileName);
    default:
      std::cerr << "Error: Unknown RGBA component type." << std::endl;
      return false;
    }
    break;
  default:
    std::cerr << "Error: Unknown pixel type." << std::endl;
    return false;
  }

  return false; // Not reached
}

template<typename PixelType>
bool StandardizeBValueHelper(const std::string &strFileName) {
  typedef itk::Image<PixelType, 2> ImageType;

  typename ImageType::Pointer p_clSlice = LoadDicomImage<PixelType, 2>(strFileName);

  if (!p_clSlice) {
    std::cerr << "Error: Failed to load image slice." << std::endl;
    return false;
  }

  itk::MetaDataDictionary clDicomTags = p_clSlice->GetMetaDataDictionary();

  std::string strBValue = ComputeDiffusionBValue(clDicomTags);

  if (strBValue.empty()) {
    std::cerr << "Error: Could not determine diffusion b-value (not a diffusion scan?)." << std::endl;
    return false;
  }

  std::cout << "Info: Diffusion b-value = " << strBValue << std::endl;

  itk::EncapsulateMetaData(clDicomTags, "0018|9087", strBValue);

  p_clSlice->SetMetaDataDictionary(clDicomTags);

  std::cout << "Info: Saving standardized image to '" << strFileName << "' ..." << std::endl;

  if (!SaveDicomSlice<PixelType>(p_clSlice, strFileName)) {
    std::cerr << "Error: Failed to save image." << std::endl;
    return false;
  }

  return true;
}
