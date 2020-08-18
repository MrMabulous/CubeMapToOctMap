/*
 * Copyright(c) 2020 Matthias Bühlmann, Mabulous GmbH. http://www.mabulous.com
*/

#ifndef IMAGE_FILTER_H
#define IMAGE_FILTER_H

#include "IlmBase/Imath/ImathVec.h"

class Filter {
 public:
  // Evaluate the filter for the given position relative to it's center.
  virtual float Eval(const Imath::V2f& position) const = 0;

  // Return the radius of the filter. This is a bounding circle around the
  // support of the filter.
  virtual float GetRadius() const = 0;
};

class MitchellFilter : public Filter {
 public:
  // Constructs a Mitchell filter with the given parameters B and C.
  // B = 0, C = 1 is the cubic B-spline.
  // B = 0, is the family of cardinal splines.
  // B = 0, C = 0.5 is the Catmull-Rom spline.
  // The authors of the original paper suggest B + 2C = 1 as good parameters.
  // In particular B = C = 1/3.
  MitchellFilter(float b, float c) : b_(b), c_(c) {}

  // Default constructor, B = C = 1/3.
  MitchellFilter() : b_(1.0f / 3.0f), c_(1.0f / 3.0f) {}

  float Eval(const Imath::V2f& position) const override {
    const float d = position.length();
    const float d2 = d * d;
    const float d3 = d2 * d;
    if (d < 1.0f) {
      return ((12.0f - 9.0f * b_ - 6.0f * c_) * d3 +
              (-18.0f + 12.0f * b_ + 6.0f * c_) * d2 +
              (6.0f - 2.0f * b_)) /
             6.0f;
    } else if ((d >= 1.0f) && (d < 2.0f)) {
      return ((-b_ - 6.0f * c_) * d3 +
              (6.0f * b_ + 30.0f * c_) * d2 +
              (-12.0f * b_ - 48.0f * c_) * d +
              (8.0f * b_ + 24.0f * c_)) /
             6.0f;
    } else {
      return 0.0f;
    }
  }

  float GetRadius() const override { return 2.0f; }

 private:
  // The two constants B and C defining the filter's shape.
  const float b_;
  const float c_;
};

class GaussianFilter : public Filter {
 public:
  // Constructs a gaussian filter with the given standard deviation sigma and
  // radius. The gaussian is truncated at distance radius and shifted such
  // that f(radius) = 0. As a rule of thumb, radius = 3 * sigma is a reasonable
  // choice for the cut-off.
  GaussianFilter(float sigma, float radius)
      : radius_(radius),
        a_(1.0f / (std::sqrt(2 * M_PI) * sigma)),
        b_(-1.0f / (2.0f * sigma * sigma)),
        c_(-a_ * std::exp(radius * radius * b_)) {}

  // Default constructor with radius = 2.0, sigma = 2.0/3
  GaussianFilter() : GaussianFilter(2.0f/3, 2.0f) {}
  

  float Eval(const Imath::V2f& position) const override {
    const float d2 = position.length2();;
    if (d2 >= radius_ * radius_) {
      return 0.0f;
    }
    return a_ * std::exp(d2 * b_) + c_;
  }

  float GetRadius() const override { return radius_; }

 private:
  const float radius_;
  const float a_;
  const float b_;
  const float c_;
};


#endif  // IMAGE_FILTER_H