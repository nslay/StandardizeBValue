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

#ifndef COMMON_H
#define COMMON_H

#include <cstring>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "itkImage.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "vnl/vnl_vector_fixed.h"
#include "vnl/vnl_cross.h"

void Trim(std::string &strString);
std::vector<std::string> SplitString(const std::string &strValue, const std::string &strDelim);

bool FileExists(const std::string &strPath);
bool IsFolder(const std::string &strPath);
bool RmDir(const std::string &strPath);
bool MkDir(const std::string &strPath);
bool Unlink(const std::string &strPath);
bool Copy(const std::string &strFrom, const std::string &strTo, bool bReplace = false);
bool Rename(const std::string &strFrom, const std::string &strTo, bool bReplace = false);
void USleep(unsigned int uiMicroSeconds);

std::string BaseName(std::string strPath);
std::string DirName(std::string strPath);

void SanitizeFileName(std::string &strFileName); // Does NOT operate on paths
void FindFiles(const char *p_cDir, const char *p_cPattern, std::vector<std::string> &vFiles, bool bRecursive = false);
void FindFolders(const char *p_cDir, const char *p_cPattern, std::vector<std::string> &vFolders, bool bRecursive = false);
void FindDicomFolders(const char *p_cDir, const char *p_cPattern, std::vector<std::string> &vFolders, bool bRecursive = false);

// Use LoadImg since Windows #defines LoadImage ... lame
template<typename PixelType, unsigned int Dimension>
typename itk::Image<PixelType, Dimension>::Pointer LoadImg(const std::string &strPath);

template<typename PixelType, unsigned int Dimension>
bool SaveImg(typename itk::Image<PixelType, Dimension>::Pointer p_clImage, const std::string &strPath, bool bCompress = true);

// Append a singleton dimension and possibly set relevant 3D information for DICOM
template<typename PixelType>
typename itk::Image<PixelType, 3>::Pointer PromoteSlice(typename itk::Image<PixelType, 2>::Pointer p_clSlice, bool bDeepCopy = false);

// Save a DICOM slice and keep all UIDs and private tags
template<typename PixelType>
bool SaveDicomSlice(typename itk::Image<PixelType, 2>::Pointer p_clImage, const std::string &strPath, bool bCompress = false);

template<typename PixelType, unsigned int Dimension>
typename itk::Image<PixelType, Dimension>::Pointer LoadDicomImage(const std::string &strPath, const std::string &strSeriesUID = std::string());

template<typename PixelType, unsigned int Dimension>
typename itk::Image<PixelType, Dimension>::Pointer LoadImg(const std::string &strPath) {
  typedef itk::Image<PixelType, Dimension> ImageType;
  typedef itk::ImageFileReader<ImageType> ReaderType;

  typename ReaderType::Pointer p_clReader = ReaderType::New();

  p_clReader->SetFileName(strPath);

  try {
    p_clReader->Update();
  }
  catch (itk::ExceptionObject &e) {
    std::cerr << "Error: " << e << std::endl;
    return typename ImageType::Pointer();
  }

  return p_clReader->GetOutput();
}

template<typename PixelType, unsigned int Dimension>
bool SaveImg(typename itk::Image<PixelType, Dimension>::Pointer p_clImage, const std::string &strPath, bool bCompress) {
  typedef itk::Image<PixelType, Dimension> ImageType;
  typedef itk::ImageFileWriter<ImageType> WriterType;

  typename WriterType::Pointer p_clWriter = WriterType::New();

  p_clWriter->SetFileName(strPath);
  p_clWriter->SetUseCompression(bCompress);
  p_clWriter->SetInput(p_clImage);

  try {
    p_clWriter->Update();
  }
  catch (itk::ExceptionObject &e) {
    std::cerr << "Error: " << e << std::endl;
    return false;
  }

  return true;
}

template<typename PixelType>
typename itk::Image<PixelType, 3>::Pointer PromoteSlice(typename itk::Image<PixelType, 2>::Pointer p_clSlice, bool bDeepCopy) {
  typedef itk::Image<PixelType, 2> ImageType;
  typedef itk::Image<PixelType, 3> ImageType3D;
  typedef typename ImageType::SizeType SizeType;
  typedef typename ImageType::SpacingType SpacingType;
  typedef typename ImageType::PointType PointType;
  typedef typename ImageType3D::SizeType SizeType3D;
  typedef typename ImageType3D::SpacingType SpacingType3D;
  typedef typename ImageType3D::PointType PointType3D;
  typedef typename ImageType3D::DirectionType DirectionType3D;
  
  if (!p_clSlice || !p_clSlice->GetBufferPointer())
    return typename ImageType3D::Pointer();
  
  const SizeType &clSize = p_clSlice->GetBufferedRegion().GetSize();
  const SpacingType &clSpacing = p_clSlice->GetSpacing();
  const PointType &clOrigin = p_clSlice->GetOrigin();
  
  SizeType3D clSize3D;
  clSize3D[0] = clSize[0];
  clSize3D[1] = clSize[1];
  clSize3D[2] = 1;
  
  SpacingType3D clSpacing3D;
  clSpacing3D[0] = clSpacing[0];
  clSpacing3D[1] = clSpacing[1];
  clSpacing3D[2] = 1; // Default
  
  PointType3D clOrigin3D;
  clOrigin3D[0] = clOrigin[0];
  clOrigin3D[1] = clOrigin[1];
  clOrigin3D[2] = 0; // Default
  
  itk::MetaDataDictionary clDicomTags = p_clSlice->GetMetaDataDictionary();
  
  std::string strSpacingBetweenSlices; // 0018|0088
  
  if (!itk::ExposeMetaData(clDicomTags, "0018|0088", strSpacingBetweenSlices))
    return typename ImageType3D::Pointer(); // Not a DICOM, at least not one with Spacing Between Slices tag
 
  Trim(strSpacingBetweenSlices);
 
  char *p = nullptr;
  const float fSpacingBetweenSlices = (float)strtod(strSpacingBetweenSlices.c_str(), &p);
  
  if (*p != '\0')
    return typename ImageType3D::Pointer(); // Could not interpret this tag?
  
  clSpacing3D[2] = fSpacingBetweenSlices;
  
  typename ImageType3D::Pointer p_clSlice3D = ImageType3D::New();
  
  p_clSlice3D->SetRegions(clSize3D);
  p_clSlice3D->SetSpacing(clSpacing3D);
  p_clSlice3D->SetOrigin(clOrigin3D);
  p_clSlice3D->SetMetaDataDictionary(clDicomTags);
  
  if (!bDeepCopy) {
    p_clSlice3D->GetPixelContainer()->SetImportPointer(p_clSlice->GetBufferPointer(), (size_t)clSize[0]*clSize[1], false);
  }
  else {
   const PixelType * const p_inBuffer = p_clSlice->GetBufferPointer();
   p_clSlice3D->Allocate();
   std::copy(p_inBuffer, p_inBuffer + (size_t)clSize[0]*clSize[1], p_clSlice3D->GetBufferPointer());
  }
  
  p_clSlice3D->SetSpacing(clSpacing3D);
  
  // Now pull out all the other 3D tags
  std::string strImagePositionPatient; // 0020|0032
  std::string strImageOrientationPatient; // 0020|0037
  
  if (!itk::ExposeMetaData(clDicomTags, "0020|0032", strImagePositionPatient))
    return p_clSlice3D; // Uhh?
  
  Trim(strImagePositionPatient);
  
  vnl_vector_fixed<float, 3> clX, clY, clZ; // Temporaries for position and orientation calculation
  
  if (sscanf(strImagePositionPatient.c_str(), "%f\\%f\\%f", &clX[0], &clX[1], &clX[2]) != 3)
    return p_clSlice3D;
  
  clOrigin3D[0] = clX[0];
  clOrigin3D[1] = clX[1];
  clOrigin3D[2] = clX[2];
  
  p_clSlice3D->SetOrigin(clOrigin3D);
  
  if (!itk::ExposeMetaData(clDicomTags, "0020|0037", strImageOrientationPatient))
    return p_clSlice3D; // Uhh?
  
  Trim(strImageOrientationPatient);
  
  if (sscanf(strImageOrientationPatient.c_str(), "%f\\%f\\%f\\%f\\%f\\%f", &clX[0], &clX[1], &clX[2], &clY[0], &clY[1], &clY[2]) != 6)
    return p_clSlice3D;
  
  clZ = vnl_cross_3d(clX, clY);
  
  DirectionType3D clR;
  
  clR(0,0) = clX[0];
  clR(1,0) = clX[1];
  clR(2,0) = clX[2];
  
  clR(0,1) = clY[0];
  clR(1,1) = clY[1];
  clR(2,1) = clY[2];
  
  clR(0,2) = clZ[0];
  clR(1,2) = clZ[1];
  clR(2,2) = clZ[2];
  
  p_clSlice3D->SetDirection(clR);
  
  return p_clSlice3D;
}

template<typename PixelType>
bool SaveDicomSlice(typename itk::Image<PixelType, 2>::Pointer p_clSlice, const std::string &strFileName, bool bCompress) {
  typedef itk::Image<PixelType, 2> ImageType;
  typedef itk::Image<PixelType, 3> ImageType3D;
  typedef itk::GDCMImageIO ImageIOType;
  typedef itk::ImageFileWriter<ImageType> WriterType;
  typedef itk::ImageFileWriter<ImageType3D> WriterType3D;
  
  if (!p_clSlice)
    return false;
  
  typename ImageType3D::Pointer p_clSlice3D = PromoteSlice<PixelType>(p_clSlice);
  
  ImageIOType::Pointer p_clImageIO = ImageIOType::New();
  
  p_clImageIO->KeepOriginalUIDOn();
  p_clImageIO->LoadPrivateTagsOn();
  p_clImageIO->SetUseCompression(bCompress); // Maybe not needed?
  
  if (!p_clSlice3D) {
    // Normal 2D DICOM?
    typename WriterType::Pointer p_clWriter = WriterType::New();
    
    p_clWriter->SetImageIO(p_clImageIO);
    p_clWriter->SetUseCompression(bCompress);
    p_clWriter->SetFileName(strFileName);
    p_clWriter->SetInput(p_clSlice);
    
    try {
      p_clWriter->Update();
    }
    catch (itk::ExceptionObject &e) {
      std::cerr << "Error: " << e << std::endl;
      return false;
    }
    
    return true;
  }
  
  typename WriterType3D::Pointer p_clWriter3D = WriterType3D::New();
  
  p_clWriter3D->SetImageIO(p_clImageIO);
  p_clWriter3D->SetUseCompression(bCompress);
  p_clWriter3D->SetFileName(strFileName);
  p_clWriter3D->SetInput(p_clSlice3D);
  
  try {
    p_clWriter3D->Update();
  }
  catch (itk::ExceptionObject &e) {
    std::cerr << "Error: " << e << std::endl;
    return false;
  }
  
  return true;
}

template<typename PixelType, unsigned int Dimension>
typename itk::Image<PixelType, Dimension>::Pointer LoadDicomImage(const std::string &strPath, const std::string &strSeriesUID) {
  typedef itk::Image<PixelType, Dimension> ImageType;
  typedef itk::GDCMImageIO ImageIOType;
  typedef itk::GDCMSeriesFileNames FileNameGeneratorType;

  if (!FileExists(strPath)) // File or folder must exist
    return typename ImageType::Pointer();

  ImageIOType::Pointer p_clImageIO = ImageIOType::New();

  p_clImageIO->LoadPrivateTagsOn();
  p_clImageIO->KeepOriginalUIDOn();

  if (Dimension == 2) {
    // Read a 2D image
    typedef itk::ImageFileReader<ImageType> ReaderType;

    if (IsFolder(strPath)) // Must be a file
      return typename ImageType::Pointer();
    
    if (!p_clImageIO->CanReadFile(strPath.c_str()))
      return typename ImageType::Pointer();

    typename ReaderType::Pointer p_clReader = ReaderType::New();

    p_clReader->SetImageIO(p_clImageIO);
    p_clReader->SetFileName(strPath);

    try {
      p_clReader->Update();
    }
    catch (itk::ExceptionObject &e) {
      std::cerr << "Error: " << e << std::endl;
      return typename ImageType::Pointer();
    }

    return p_clReader->GetOutput();
  }

  // Passed a file, read the series UID (ignore the one provided, if any)
  if (!IsFolder(strPath)) {

    if (!p_clImageIO->CanReadFile(strPath.c_str()))
      return typename ImageType::Pointer();

    p_clImageIO->SetFileName(strPath.c_str());

    try {
      p_clImageIO->ReadImageInformation();
    }
    catch (itk::ExceptionObject &e) {
      std::cerr << "Error: " << e << std::endl;
      return typename ImageType::Pointer();
    }

    const itk::MetaDataDictionary &clDicomTags = p_clImageIO->GetMetaDataDictionary();

    std::string strTmpSeriesUID;
    if (!itk::ExposeMetaData(clDicomTags, "0020|000e", strTmpSeriesUID))
      return typename ImageType::Pointer();

    Trim(strTmpSeriesUID);

    return LoadDicomImage<PixelType, Dimension>(DirName(strPath), strTmpSeriesUID); // Call this function again
  }

  FileNameGeneratorType::Pointer p_clFileNameGenerator = FileNameGeneratorType::New();

  // Use the ACTUAL series UID ... not some custom ITK concatenations of lots of junk.
  p_clFileNameGenerator->SetUseSeriesDetails(false); 
  p_clFileNameGenerator->SetDirectory(strPath);

  if (strSeriesUID.empty()) {
    // Passed a folder but no series UID ... pick the first series UID
    const FileNameGeneratorType::SeriesUIDContainerType &vSeriesUIDs = p_clFileNameGenerator->GetSeriesUIDs();

    if (vSeriesUIDs.empty())
      return typename ImageType::Pointer();

    // Use first series UID
    return LoadDicomImage<PixelType, Dimension>(strPath, vSeriesUIDs[0]);
  }

  const FileNameGeneratorType::FileNamesContainerType &vDicomFiles = p_clFileNameGenerator->GetFileNames(strSeriesUID);

  if (vDicomFiles.empty())
    return typename ImageType::Pointer();

  // Read 3D or higher (but 4D probably doesn't work correctly)
  typedef itk::ImageSeriesReader<ImageType> ReaderType;

  typename ReaderType::Pointer p_clReader = ReaderType::New();

  p_clReader->SetImageIO(p_clImageIO);
  p_clReader->SetFileNames(vDicomFiles);

  try {
    p_clReader->Update();
  }
  catch (itk::ExceptionObject &e) {
    std::cerr << "Error: " << e << std::endl;
    return typename ImageType::Pointer();
  }

  return p_clReader->GetOutput();
}

#endif // !COMMON_H
