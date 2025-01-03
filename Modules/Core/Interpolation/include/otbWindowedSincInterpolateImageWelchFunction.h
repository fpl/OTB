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

#ifndef otbWindowedSincInterpolateImageWelchFunction_h
#define otbWindowedSincInterpolateImageWelchFunction_h

#include "otbWindowedSincInterpolateImageFunctionBase.h"
#include "vnl/vnl_math.h"

namespace otb
{

namespace Function
{
/**
 * \class WelchWindowFunction
 * \brief Window function for sinc interpolation.
 * \f[ w(x) = 1 - ( \frac{x^2}{m^2} ) \f]
 * \sa WindowedSincInterpolateImageWelchFunction
 *
 * \ingroup OTBInterpolation
 */
template <class TInput = double, class TOutput = double>
class WelchWindowFunction
{
public:
  WelchWindowFunction() : m_Radius(1), m_Factor(1)
  {
  } // default radius is 1 at construction
  void SetRadius(unsigned int radius)
  {
    m_Radius = radius;
    m_Factor = 1 / static_cast<double>(radius * radius);
  }
  unsigned int GetRadius() const
  {
    return m_Radius;
  }
  double GetFactor()
  {
    return m_Factor;
  }

  inline TOutput operator()(const TInput& A) const
  {
    double x    = static_cast<double>(A);
    double px   = CONST_PI * x;
    double temp = 1.0 - x * m_Factor * x;
    return (x == 0.0) ? static_cast<TOutput>(temp) : static_cast<TOutput>(temp * std::sin(px) / px);
  }

private:
  unsigned int m_Radius;
  // Equal to \f$ \frac{1}{m^2} \f$
  double m_Factor;
};
} // namespace Function

/**
 * \class WindowedSincInterpolateImageWelchFunction
 * \brief Use the WindowedSincInterpolateImageFunctionBase with a Welch Function.
 *
 * \sa GenericInterpolatorImageFunction
 * \sa Function::GaussianWindowFunction
 * \sa Function::HammingWindowFunction
 * \sa Function::CosineWindowFunction
 * \sa Function::WelchWindowFunction
 * \sa Function::LanczosWindowFunction
 * \sa Function::BlackmanWindowFunction
 * \ingroup ImageFunctionBases ImageInterpolators
 *
 * \ingroup OTBInterpolation
 */
template <class TInputImage, class TBoundaryCondition = itk::ConstantBoundaryCondition<TInputImage>, class TCoordRep = double,
          class TInputInterpolator = double, class TOutputInterpolator = double>
class ITK_EXPORT WindowedSincInterpolateImageWelchFunction
    : public WindowedSincInterpolateImageFunctionBase<TInputImage, typename Function::WelchWindowFunction<TInputInterpolator, TOutputInterpolator>,
                                                      TBoundaryCondition, TCoordRep>
{
public:
  /** Standard class typedefs. */
  typedef WindowedSincInterpolateImageWelchFunction Self;
  typedef WindowedSincInterpolateImageFunctionBase<TInputImage, typename Function::WelchWindowFunction<TInputInterpolator, TOutputInterpolator>,
                                                   TBoundaryCondition, TCoordRep>
                                        Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(WindowedSincInterpolateImageWelchFunction, WindowedSincInterpolateImageFunctionBase);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Input and output images typedef definition. */
  typedef typename Superclass::InputImageType InputImageType;
  typedef typename Superclass::OutputType     OutputType;

  /** Dimension underlying input image. */
  itkStaticConstMacro(ImageDimension, unsigned int, Superclass::ImageDimension);

  /** Superclass typedef inheritance. */
  typedef typename Superclass::IndexType           IndexType;
  typedef typename Superclass::SizeType            SizeType;
  typedef typename Superclass::RealType            RealType;
  typedef typename Superclass::IteratorType        IteratorType;
  typedef typename Superclass::ContinuousIndexType ContinuousIndexType;

protected:
  WindowedSincInterpolateImageWelchFunction(){};
  ~WindowedSincInterpolateImageWelchFunction() override
  {
  }
  void PrintSelf(std::ostream& os, itk::Indent indent) const override
  {
    Superclass::PrintSelf(os, indent);
  }

private:
  WindowedSincInterpolateImageWelchFunction(const Self&) = delete;
  void operator=(const Self&) = delete;
};

} // end namespace otb

#endif
