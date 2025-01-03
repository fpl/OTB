/*
 * Copyright (C) 2005-2024 Centre National d'Etudes Spatiales (CNES)
 *
 * This file is part of Orfeo Toolbox
 *
 *     https://www.orfeo-toolbox.org/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Wrappers
#include "otbWrapperApplicationFactory.h"
#include "otbWrapperChoiceParameter.h"
#include "otbWrapperElevationParametersHandler.h"
#include "otbWrapperMapProjectionParametersHandler.h"
#include "itkPoint.h"
#include "itkEuclideanDistanceMetric.h"
#include "otbGenericRSTransform.h"
#include "otbOGRDataSourceWrapper.h"
#include "ogrsf_frmts.h"
#include "otbDEMHandler.h"
#include "otbSensorTransformFactory.h"

namespace otb
{
namespace Wrapper
{
class RefineSensorModel : public Application
{
public:
  /** Standard class typedefs. */
  using Self = RefineSensorModel;
  using Superclass = Application;
  using Pointer = itk::SmartPointer<Self>;
  using ConstPointer = itk::SmartPointer<const Self>;

  using SensorPointType = itk::Point<double, 2>;
  using GroundPointType = itk::Point<double, 3>;
  using DistanceType = itk::Statistics::EuclideanDistanceMetric<GroundPointType>;

  using TiePointType = std::pair<SensorPointType, GroundPointType>;
  using TiePointsType = std::vector<TiePointType>;

  using RSTransformType = otb::GenericRSTransform<double, 3, 3>;

  /** Standard macro */
  itkNewMacro(Self);
  itkTypeMacro(RefineSensorModel, otb::Application);

private:
  void DoInit() override
  {
    SetName("RefineSensorModel");
    SetDescription("Perform least-square fit of a sensor model to a set of tie points");

    SetDocLongDescription(
        "This application reads an image file containing a sensor model (this model can be passed as a geom as an extended filename) and a text file containing a list of ground control point, and performs a least-square "
        "fit of the sensor model adjustable parameters to these tie points. It produces an updated image file as output, as well as an optional ground control "
        "points based statistics file and a vector file containing residues. The updated output image file can then be used to ortho-rectify the data more accurately. "
        "Plaease note that for a proper use of the application, elevation must be correctly set (including DEM and geoid file). The map parameters allows one "
        "to choose a map projection in which the accuracy will be estimated in meters.");

    AddDocTag(Tags::Geometry);

    SetDocLimitations("None");
    SetDocSeeAlso("OrthoRectification,HomologousPointsExtraction");
    SetDocAuthors("OTB-Team");

    AddParameter(ParameterType_InputImage, "in", "Input image file");
    SetParameterDescription("in", "Image containing the sensor model to refine that can be in a geom file (extended filename) or in the image");

    AddParameter(ParameterType_OutputImage, "out", "Output image file");
    SetParameterDescription("out", "Image containing the refined sensor model");

    AddParameter(ParameterType_InputFilename, "inpoints", "Input file containing tie points");
    SetParameterDescription("inpoints",
                            "Input file containing tie points. Points are stored in following format: row col lon lat. Line beginning with # are ignored.");

    AddParameter(ParameterType_OutputFilename, "outstat", "Output file containing output precision statistics");
    SetParameterDescription("outstat",
                            "Output file containing the following info: ref_lon ref_lat elevation predicted_lon predicted_lat x_error_ref(meters) "
                            "y_error_ref(meters) global_error_ref(meters) x_error(meters) y_error(meters) overall_error(meters)");
    MandatoryOff("outstat");
    DisableParameter("outstat");

    AddParameter(ParameterType_OutputFilename, "outvector", "Output vector file with residues");
    SetParameterDescription("outvector", "File containing segments representing residues");
    MandatoryOff("outvector");
    DisableParameter("outvector");

    // Build the Output Map Projection
    MapProjectionParametersHandler::AddMapProjectionParameters(this, "map");

    // Elevation
    ElevationParametersHandler::AddElevationParameters(this, "elev");

    // Doc example parameter settings
    SetDocExampleParameterValue("in", "input.tif&?geom=TheGeom.geom");
    SetDocExampleParameterValue("out", "output.tif");
    SetDocExampleParameterValue("inpoints", "points.txt");
    SetDocExampleParameterValue("map", "epsg");
    SetDocExampleParameterValue("map.epsg.code", "32631");

    SetOfficialDocLink();
  }

  void DoUpdateParameters() override
  {
    // Nothing to do here : all parameters are independent
  }

  void DoExecute() override
  {
    OGRMultiLineString mls;
    
    FloatVectorImageType::Pointer inImage = GetParameterImage("in");
    FloatVectorImageType::Pointer outImage = inImage;
    // Create sensor model from input file
    auto sensorModel_reference = otb::SensorTransformFactory::GetInstance().CreateTransform <double, 2, 3>(inImage->GetImageMetadata(), TransformDirection::FORWARD);
    auto sensorModelRefined = otb::SensorTransformFactory::GetInstance().CreateTransform <double, 2, 3>(inImage->GetImageMetadata(), TransformDirection::FORWARD);
    if (sensorModel_reference->IsValidSensorModel() == false)
    {
      otbAppLogFATAL("Invalid Model pointer sensorModel_reference == NULL!\n The metadata is invalid!");
    }
    // Setup the DEM Handler
    otb::Wrapper::ElevationParametersHandler::SetupDEMHandlerFromElevationParameters(this, "elev");

    // Parse the input file for ground control points
    std::ifstream ifs;
    ifs.open(GetParameterString("inpoints"));

    TiePointsType tiepoints;
    double x = 0.0, y = 0.0, z = 0.0, lat = 0.0, lon = 0.0;
    std::string line;

    std::istringstream iss;
    while (std::getline(ifs, line))
    {
      // Avoid commented lines or too short ones
      if (!line.empty() && line[0] != '#')
      {
        iss.str(line);
          if (! (iss >> x >> y >> lon >> lat)) {
          throw std::invalid_argument("Invalid line: \"" + iss.str() +
                                      "\". Expected 4 doubles separated"
                                      " by space or tab.");
        }
        // you must clear istringstream to reset the error state and eof flag
        // otherwise all further attempts to read line will fail
        iss.clear();

        z = otb::DEMHandler::GetInstance().GetHeightAboveEllipsoid(lon, lat);

        otbAppLogDEBUG("Adding tie point x=" << x << ", y=" << y << ", z=" << z << ", lon=" << lon << ", lat=" << lat);

        SensorPointType imagePoint;
        GroundPointType groundPoint;
        imagePoint[0] = x;
        imagePoint[1] = y;
        groundPoint[0] = lon;
        groundPoint[1] = lat;
        groundPoint[2] = z;

        tiepoints.push_back({imagePoint, groundPoint});
      }
    }
    ifs.close();
    otbAppLogINFO("Optimization in progress ...");
    double rmseRefined = 0.0;
    otb::ImageMetadata imdrefined = inImage->GetImageMetadata();
    sensorModelRefined->OptimizeParameters(imdrefined,tiepoints,rmseRefined);
    if (sensorModelRefined->IsValidSensorModel() == false)
    {
      otbAppLogFATAL("Invalid Model pointer sensorModelRefined == NULL!\n The metadata is invalid!");
    }
    otbAppLogINFO("Done.\n");
    outImage->SetImageMetadata(imdrefined);
    SetParameterOutputImage("out", outImage);

    double rmse  = 0;
    double rmsex = 0;
    double rmsey = 0;

    double meanx = 0;
    double meany = 0;

    double rmse_ref  = 0;
    double rmsex_ref = 0;
    double rmsey_ref = 0;

    double meanx_ref = 0;
    double meany_ref = 0;


    DistanceType::Pointer distance = DistanceType::New();

    RSTransformType::Pointer rsTransform = RSTransformType::New();
    rsTransform->SetOutputProjectionRef(MapProjectionParametersHandler::GetProjectionRefFromChoice(this, "map"));
    rsTransform->InstantiateTransform();

    std::ofstream ofs;
    ofs << std::fixed;
    ofs.precision(12);
    bool outStatEnabled = IsParameterEnabled("outstat");

    if (outStatEnabled)
    {
      ofs.open(GetParameterString("outstat"));
      ofs << "#ref_lon ref_lat elevation predicted_lon predicted_lat predicted_elev x_error_ref(meters) y_error_ref(meters) global_error_ref(meters) "
             "x_error(meters) y_error(meters) global_error(meters)"
          << std::endl;
    }

    size_t validPoints = 0;

    for (auto it = tiepoints.begin(); it != tiepoints.end(); ++it)
    {
      GroundPointType ref;
      auto tmpPoint_ref = sensorModel_reference->TransformPoint(it->first);
      auto tmpPoint = sensorModelRefined->TransformPoint(it->first);

      if (!(std::isfinite(tmpPoint[0]) && std::isfinite(tmpPoint[1]) && std::isfinite(tmpPoint[2])))
      {
        otbAppLogWARNING("Can't project tie point at [" << it->first[0] << "," << it->first[1] << "," << it->first[2] << "] with optimized model!");
        continue;
      }

      if (!(std::isfinite(tmpPoint_ref[0]) && std::isfinite(tmpPoint_ref[1]) && std::isfinite(tmpPoint_ref[2])))
      {
        otbAppLogWARNING("Can't project tie point at [" << it->first[0] << "," << it->first[1] << "," << it->first[2] << "] with original model!");
        continue;
      }

      tmpPoint[2] = it->second[2];
      tmpPoint_ref[2] = it->second[2];
      tmpPoint     = rsTransform->TransformPoint(tmpPoint);
      tmpPoint_ref = rsTransform->TransformPoint(tmpPoint_ref);

      ref[0] = it->second[0];
      ref[1] = it->second[1];
      ref[2] = it->second[2];

      ref = rsTransform->TransformPoint(ref);

      OGRLineString ls;
      ls.addPoint(tmpPoint[0], tmpPoint[1]);
      ls.addPoint(ref[0], ref[1]);
      mls.addGeometry(&ls);

      double gerror = distance->Evaluate(ref, tmpPoint);
      double xerror = ref[0] - tmpPoint[0];
      double yerror = ref[1] - tmpPoint[1];

      double gerror_ref = distance->Evaluate(ref, tmpPoint_ref);
      double xerror_ref = ref[0] - tmpPoint_ref[0];
      double yerror_ref = ref[1] - tmpPoint_ref[1];


      if (outStatEnabled)
        ofs << ref[0] << "\t" << ref[1] << "\t" << it->first[2] << "\t" << tmpPoint[0] << "\t" << tmpPoint[1] << "\t" << tmpPoint[2] << "\t" << xerror_ref
            << "\t" << yerror_ref << "\t" << gerror_ref << "\t" << xerror << "\t" << yerror << "\t" << gerror << "\n";

      rmse += gerror * gerror;
      rmsex += xerror * xerror;
      rmsey += yerror * yerror;

      meanx += xerror;
      meany += yerror;

      rmse_ref += gerror_ref * gerror_ref;
      rmsex_ref += xerror_ref * xerror_ref;
      rmsey_ref += yerror_ref * yerror_ref;

      meanx_ref += xerror_ref;
      meany_ref += yerror_ref;

      validPoints++;
    }

    if (!validPoints)
    {
      otbAppLogFATAL("No valid points to compute RMSE !");
    }

    rmse /= validPoints;

    rmsex /= validPoints;

    rmsey /= validPoints;


    meanx /= validPoints;
    meany /= validPoints;


    rmse_ref /= validPoints;

    rmsex_ref /= validPoints;

    rmsey_ref /= validPoints;


    meanx_ref /= validPoints;
    meany_ref /= validPoints;


    double stdevx = std::sqrt(rmsex - meanx * meanx);
    double stdevy = std::sqrt(rmsey - meany * meany);

    double stdevx_ref = std::sqrt(rmsex_ref - meanx_ref * meanx_ref);
    double stdevy_ref = std::sqrt(rmsey_ref - meany_ref * meany_ref);


    rmse  = std::sqrt(rmse);
    rmsex = std::sqrt(rmsex);
    rmsey = std::sqrt(rmsey);

    rmse_ref  = std::sqrt(rmse_ref);
    rmsex_ref = std::sqrt(rmsex_ref);
    rmsey_ref = std::sqrt(rmsey_ref);

    otbAppLogINFO("Estimation of input geom file accuracy: ");
    otbAppLogINFO("Overall Root Mean Square Error: " << rmse_ref << " meters");
    otbAppLogINFO("X Mean Error: " << meanx_ref << " meters");
    otbAppLogINFO("X standard deviation: " << stdevx_ref << " meters");
    otbAppLogINFO("X Root Mean Square Error: " << rmsex_ref << " meters");
    otbAppLogINFO("Y Mean Error: " << meany_ref << " meters");
    otbAppLogINFO("Y standard deviation: " << stdevy_ref << " meters");
    otbAppLogINFO("Y Root Mean Square Error: " << rmsey_ref << " meters\n");

    otbAppLogINFO("Estimation of final accuracy: ");

    otbAppLogINFO("Overall Root Mean Square Error: " << rmseRefined << " meters");
    otbAppLogINFO("X Mean Error: " << meanx << " meters");
    otbAppLogINFO("X standard deviation: " << stdevx << " meters");
    otbAppLogINFO("X Root Mean Square Error: " << rmsex << " meters");
    otbAppLogINFO("Y Mean Error: " << meany << " meters");
    otbAppLogINFO("Y standard deviation: " << stdevy << " meters");
    otbAppLogINFO("Y Root Mean Square Error: " << rmsey << " meters");


    if (IsParameterEnabled("outstat"))
      ofs.close();


    if (IsParameterEnabled("outvector"))
    {
      // Create the datasource (for matches export)
      otb::ogr::Layer               layer(nullptr, false);
      otb::ogr::DataSource::Pointer ogrDS;

      ogrDS                       = otb::ogr::DataSource::New(GetParameterString("outvector"), otb::ogr::DataSource::Modes::Overwrite);
      std::string         projref = MapProjectionParametersHandler::GetProjectionRefFromChoice(this, "map");
      OGRSpatialReference oSRS(projref.c_str());

      // and create the layer
      layer                = ogrDS->CreateLayer("matches", &oSRS, wkbMultiLineString);
      OGRFeatureDefn& defn = layer.GetLayerDefn();
      ogr::Feature    feature(defn);

      feature.SetGeometry(&mls);
      layer.CreateFeature(feature);
    }
  }
};
}
}

OTB_APPLICATION_EXPORT(otb::Wrapper::RefineSensorModel)
