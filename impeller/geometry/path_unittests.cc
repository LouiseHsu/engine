// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"

#include "flutter/testing/testing.h"
#include "impeller/geometry/geometry_asserts.h"
#include "impeller/geometry/path.h"
#include "impeller/geometry/path_builder.h"

namespace impeller {
namespace testing {

TEST(PathTest, CubicPathComponentPolylineDoesNotIncludePointOne) {
  CubicPathComponent component({10, 10}, {20, 35}, {35, 20}, {40, 40});
  std::vector<Point> polyline;
  component.AppendPolylinePoints(1.0f, polyline);
  ASSERT_NE(polyline.front().x, 10);
  ASSERT_NE(polyline.front().y, 10);
  ASSERT_EQ(polyline.back().x, 40);
  ASSERT_EQ(polyline.back().y, 40);
}

TEST(PathTest, PathCreatePolyLineDoesNotDuplicatePoints) {
  PathBuilder builder;
  builder.MoveTo({10, 10});
  builder.LineTo({20, 20});
  builder.LineTo({30, 30});
  builder.MoveTo({40, 40});
  builder.LineTo({50, 50});

  auto polyline = builder.TakePath().CreatePolyline(1.0f);

  ASSERT_EQ(polyline.contours.size(), 2u);
  ASSERT_EQ(polyline.points->size(), 5u);
  ASSERT_EQ(polyline.GetPoint(0).x, 10);
  ASSERT_EQ(polyline.GetPoint(1).x, 20);
  ASSERT_EQ(polyline.GetPoint(2).x, 30);
  ASSERT_EQ(polyline.GetPoint(3).x, 40);
  ASSERT_EQ(polyline.GetPoint(4).x, 50);
}

TEST(PathTest, PathBuilderSetsCorrectContourPropertiesForAddCommands) {
  // Closed shapes.
  {
    Path path = PathBuilder{}.AddCircle({100, 100}, 50).TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, Point(100, 50));
    ASSERT_TRUE(contour.is_closed);
  }

  {
    Path path =
        PathBuilder{}.AddOval(Rect::MakeXYWH(100, 100, 100, 100)).TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, Point(150, 100));
    ASSERT_TRUE(contour.is_closed);
  }

  {
    Path path =
        PathBuilder{}.AddRect(Rect::MakeXYWH(100, 100, 100, 100)).TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, Point(100, 100));
    ASSERT_TRUE(contour.is_closed);
  }

  {
    Path path = PathBuilder{}
                    .AddRoundedRect(Rect::MakeXYWH(100, 100, 100, 100), 10)
                    .TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, Point(110, 100));
    ASSERT_TRUE(contour.is_closed);
  }

  // Open shapes.
  {
    Point p(100, 100);
    Path path = PathBuilder{}.AddLine(p, {200, 100}).TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, p);
    ASSERT_FALSE(contour.is_closed);
  }

  {
    Path path =
        PathBuilder{}
            .AddCubicCurve({100, 100}, {100, 50}, {100, 150}, {200, 100})
            .TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, Point(100, 100));
    ASSERT_FALSE(contour.is_closed);
  }

  {
    Path path = PathBuilder{}
                    .AddQuadraticCurve({100, 100}, {100, 50}, {200, 100})
                    .TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, Point(100, 100));
    ASSERT_FALSE(contour.is_closed);
  }
}

TEST(PathTest, PathCreatePolylineGeneratesCorrectContourData) {
  Path::Polyline polyline = PathBuilder{}
                                .AddLine({100, 100}, {200, 100})
                                .MoveTo({100, 200})
                                .LineTo({150, 250})
                                .LineTo({200, 200})
                                .Close()
                                .TakePath()
                                .CreatePolyline(1.0f);
  ASSERT_EQ(polyline.points->size(), 6u);
  ASSERT_EQ(polyline.contours.size(), 2u);
  ASSERT_EQ(polyline.contours[0].is_closed, false);
  ASSERT_EQ(polyline.contours[0].start_index, 0u);
  ASSERT_EQ(polyline.contours[1].is_closed, true);
  ASSERT_EQ(polyline.contours[1].start_index, 2u);
}

TEST(PathTest, PolylineGetContourPointBoundsReturnsCorrectRanges) {
  Path::Polyline polyline = PathBuilder{}
                                .AddLine({100, 100}, {200, 100})
                                .MoveTo({100, 200})
                                .LineTo({150, 250})
                                .LineTo({200, 200})
                                .Close()
                                .TakePath()
                                .CreatePolyline(1.0f);
  size_t a1, a2, b1, b2;
  std::tie(a1, a2) = polyline.GetContourPointBounds(0);
  std::tie(b1, b2) = polyline.GetContourPointBounds(1);
  ASSERT_EQ(a1, 0u);
  ASSERT_EQ(a2, 2u);
  ASSERT_EQ(b1, 2u);
  ASSERT_EQ(b2, 6u);
}

TEST(PathTest, PathAddRectPolylineHasCorrectContourData) {
  Path::Polyline polyline = PathBuilder{}
                                .AddRect(Rect::MakeLTRB(50, 60, 70, 80))
                                .TakePath()
                                .CreatePolyline(1.0f);
  ASSERT_EQ(polyline.contours.size(), 1u);
  ASSERT_TRUE(polyline.contours[0].is_closed);
  ASSERT_EQ(polyline.contours[0].start_index, 0u);
  ASSERT_EQ(polyline.points->size(), 5u);
  ASSERT_EQ(polyline.GetPoint(0), Point(50, 60));
  ASSERT_EQ(polyline.GetPoint(1), Point(70, 60));
  ASSERT_EQ(polyline.GetPoint(2), Point(70, 80));
  ASSERT_EQ(polyline.GetPoint(3), Point(50, 80));
  ASSERT_EQ(polyline.GetPoint(4), Point(50, 60));
}

TEST(PathTest, PathPolylineDuplicatesAreRemovedForSameContour) {
  Path::Polyline polyline =
      PathBuilder{}
          .MoveTo({50, 50})
          .LineTo({50, 50})  // Insert duplicate at beginning of contour.
          .LineTo({100, 50})
          .LineTo({100, 50})  // Insert duplicate at contour join.
          .LineTo({100, 100})
          .Close()  // Implicitly insert duplicate {50, 50} across contours.
          .LineTo({0, 50})
          .LineTo({0, 100})
          .LineTo({0, 100})  // Insert duplicate at end of contour.
          .TakePath()
          .CreatePolyline(1.0f);
  ASSERT_EQ(polyline.contours.size(), 2u);
  ASSERT_EQ(polyline.contours[0].start_index, 0u);
  ASSERT_TRUE(polyline.contours[0].is_closed);
  ASSERT_EQ(polyline.contours[1].start_index, 4u);
  ASSERT_FALSE(polyline.contours[1].is_closed);
  ASSERT_EQ(polyline.points->size(), 7u);
  ASSERT_EQ(polyline.GetPoint(0), Point(50, 50));
  ASSERT_EQ(polyline.GetPoint(1), Point(100, 50));
  ASSERT_EQ(polyline.GetPoint(2), Point(100, 100));
  ASSERT_EQ(polyline.GetPoint(3), Point(50, 50));
  ASSERT_EQ(polyline.GetPoint(4), Point(50, 50));
  ASSERT_EQ(polyline.GetPoint(5), Point(0, 50));
  ASSERT_EQ(polyline.GetPoint(6), Point(0, 100));
}

TEST(PathTest, PolylineBufferReuse) {
  auto point_buffer = std::make_unique<std::vector<Point>>();
  auto point_buffer_address = reinterpret_cast<uintptr_t>(point_buffer.get());
  Path::Polyline polyline =
      PathBuilder{}
          .MoveTo({50, 50})
          .LineTo({100, 100})
          .TakePath()
          .CreatePolyline(
              1.0f, std::move(point_buffer),
              [point_buffer_address](
                  Path::Polyline::PointBufferPtr point_buffer) {
                ASSERT_EQ(point_buffer->size(), 0u);
                ASSERT_EQ(point_buffer_address,
                          reinterpret_cast<uintptr_t>(point_buffer.get()));
              });
}

TEST(PathTest, PolylineFailsWithNullptrBuffer) {
  EXPECT_DEATH_IF_SUPPORTED(PathBuilder{}
                                .MoveTo({50, 50})
                                .LineTo({100, 100})
                                .TakePath()
                                .CreatePolyline(1.0f, nullptr),
                            "");
}

TEST(PathTest, PathShifting) {
  PathBuilder builder{};
  auto path =
      builder.AddLine(Point(0, 0), Point(10, 10))
          .AddQuadraticCurve(Point(10, 10), Point(15, 15), Point(20, 20))
          .AddCubicCurve(Point(20, 20), Point(25, 25), Point(-5, -5),
                         Point(30, 30))
          .Close()
          .Shift(Point(1, 1))
          .TakePath();

  ContourComponent contour;
  LinearPathComponent linear;
  QuadraticPathComponent quad;
  CubicPathComponent cubic;

  ASSERT_TRUE(path.GetContourComponentAtIndex(0, contour));
  ASSERT_TRUE(path.GetLinearComponentAtIndex(1, linear));
  ASSERT_TRUE(path.GetQuadraticComponentAtIndex(3, quad));
  ASSERT_TRUE(path.GetCubicComponentAtIndex(5, cubic));

  EXPECT_EQ(contour.destination, Point(1, 1));

  EXPECT_EQ(linear.p1, Point(1, 1));
  EXPECT_EQ(linear.p2, Point(11, 11));

  EXPECT_EQ(quad.cp, Point(16, 16));
  EXPECT_EQ(quad.p1, Point(11, 11));
  EXPECT_EQ(quad.p2, Point(21, 21));

  EXPECT_EQ(cubic.cp1, Point(26, 26));
  EXPECT_EQ(cubic.cp2, Point(-4, -4));
  EXPECT_EQ(cubic.p1, Point(21, 21));
  EXPECT_EQ(cubic.p2, Point(31, 31));
}

TEST(PathTest, PathBuilderWillComputeBounds) {
  PathBuilder builder;
  auto path_1 = builder.AddLine({0, 0}, {1, 1}).TakePath();

  ASSERT_EQ(path_1.GetBoundingBox().value_or(Rect::MakeMaximum()),
            Rect::MakeLTRB(0, 0, 1, 1));

  auto path_2 = builder.AddLine({-1, -1}, {1, 1}).TakePath();

  // Verify that PathBuilder recomputes the bounds.
  ASSERT_EQ(path_2.GetBoundingBox().value_or(Rect::MakeMaximum()),
            Rect::MakeLTRB(-1, -1, 1, 1));

  // PathBuilder can set the bounds to whatever it wants
  auto path_3 = builder.AddLine({0, 0}, {1, 1})
                    .SetBounds(Rect::MakeLTRB(0, 0, 100, 100))
                    .TakePath();

  ASSERT_EQ(path_3.GetBoundingBox().value_or(Rect::MakeMaximum()),
            Rect::MakeLTRB(0, 0, 100, 100));
}

TEST(PathTest, PathHorizontalLine) {
  PathBuilder builder;
  auto path = builder.HorizontalLineTo(10).TakePath();

  LinearPathComponent linear;
  path.GetLinearComponentAtIndex(1, linear);

  EXPECT_EQ(linear.p1, Point(0, 0));
  EXPECT_EQ(linear.p2, Point(10, 0));
}

TEST(PathTest, PathVerticalLine) {
  PathBuilder builder;
  auto path = builder.VerticalLineTo(10).TakePath();

  LinearPathComponent linear;
  path.GetLinearComponentAtIndex(1, linear);

  EXPECT_EQ(linear.p1, Point(0, 0));
  EXPECT_EQ(linear.p2, Point(0, 10));
}

TEST(PathTest, QuadradicPath) {
  PathBuilder builder;
  auto path = builder.QuadraticCurveTo(Point(10, 10), Point(20, 20)).TakePath();

  QuadraticPathComponent quad;
  path.GetQuadraticComponentAtIndex(1, quad);

  EXPECT_EQ(quad.p1, Point(0, 0));
  EXPECT_EQ(quad.cp, Point(10, 10));
  EXPECT_EQ(quad.p2, Point(20, 20));
}

TEST(PathTest, CubicPath) {
  PathBuilder builder;
  auto path =
      builder.CubicCurveTo(Point(10, 10), Point(-10, -10), Point(20, 20))
          .TakePath();

  CubicPathComponent cubic;
  path.GetCubicComponentAtIndex(1, cubic);

  EXPECT_EQ(cubic.p1, Point(0, 0));
  EXPECT_EQ(cubic.cp1, Point(10, 10));
  EXPECT_EQ(cubic.cp2, Point(-10, -10));
  EXPECT_EQ(cubic.p2, Point(20, 20));
}

TEST(PathTest, BoundingBoxCubic) {
  PathBuilder builder;
  auto path =
      builder.AddCubicCurve({120, 160}, {25, 200}, {220, 260}, {220, 40})
          .TakePath();
  auto box = path.GetBoundingBox();
  Rect expected = Rect::MakeXYWH(93.9101, 40, 126.09, 158.862);
  ASSERT_TRUE(box.has_value());
  ASSERT_RECT_NEAR(box.value_or(Rect::MakeMaximum()), expected);
}

TEST(PathTest, BoundingBoxOfCompositePathIsCorrect) {
  PathBuilder builder;
  builder.AddRoundedRect(Rect::MakeXYWH(10, 10, 300, 300), {50, 50, 50, 50});
  auto path = builder.TakePath();
  auto actual = path.GetBoundingBox();
  Rect expected = Rect::MakeXYWH(10, 10, 300, 300);

  ASSERT_TRUE(actual.has_value());
  ASSERT_RECT_NEAR(actual.value_or(Rect::MakeMaximum()), expected);
}

TEST(PathTest, ExtremaOfCubicPathComponentIsCorrect) {
  CubicPathComponent cubic{{11.769268, 252.883148},
                           {-6.2857933, 204.356461},
                           {-4.53997231, 156.552902},
                           {17.0067291, 109.472488}};
  auto points = cubic.Extrema();

  ASSERT_EQ(points.size(), static_cast<size_t>(3));
  ASSERT_POINT_NEAR(points[2], cubic.Solve(0.455916));
}

TEST(PathTest, PathGetBoundingBoxForCubicWithNoDerivativeRootsIsCorrect) {
  PathBuilder builder;
  // Straight diagonal line.
  builder.AddCubicCurve({0, 1}, {2, 3}, {4, 5}, {6, 7});
  auto path = builder.TakePath();
  auto actual = path.GetBoundingBox();
  auto expected = Rect::MakeLTRB(0, 1, 6, 7);

  ASSERT_TRUE(actual.has_value());
  ASSERT_RECT_NEAR(actual.value_or(Rect::MakeMaximum()), expected);
}

TEST(PathTest, EmptyPath) {
  auto path = PathBuilder{}.TakePath();
  ASSERT_EQ(path.GetComponentCount(), 1u);

  ContourComponent c;
  path.GetContourComponentAtIndex(0, c);
  ASSERT_POINT_NEAR(c.destination, Point());

  Path::Polyline polyline = path.CreatePolyline(1.0f);
  ASSERT_TRUE(polyline.points->empty());
  ASSERT_TRUE(polyline.contours.empty());
}

TEST(PathTest, SimplePath) {
  PathBuilder builder;

  auto path = builder.AddLine({0, 0}, {100, 100})
                  .AddQuadraticCurve({100, 100}, {200, 200}, {300, 300})
                  .AddCubicCurve({300, 300}, {400, 400}, {500, 500}, {600, 600})
                  .TakePath();

  ASSERT_EQ(path.GetComponentCount(), 6u);
  ASSERT_EQ(path.GetComponentCount(Path::ComponentType::kLinear), 1u);
  ASSERT_EQ(path.GetComponentCount(Path::ComponentType::kQuadratic), 1u);
  ASSERT_EQ(path.GetComponentCount(Path::ComponentType::kCubic), 1u);
  ASSERT_EQ(path.GetComponentCount(Path::ComponentType::kContour), 3u);

  path.EnumerateComponents(
      [](size_t index, const LinearPathComponent& linear) {
        Point p1(0, 0);
        Point p2(100, 100);
        ASSERT_EQ(index, 1u);
        ASSERT_EQ(linear.p1, p1);
        ASSERT_EQ(linear.p2, p2);
      },
      [](size_t index, const QuadraticPathComponent& quad) {
        Point p1(100, 100);
        Point cp(200, 200);
        Point p2(300, 300);
        ASSERT_EQ(index, 3u);
        ASSERT_EQ(quad.p1, p1);
        ASSERT_EQ(quad.cp, cp);
        ASSERT_EQ(quad.p2, p2);
      },
      [](size_t index, const CubicPathComponent& cubic) {
        Point p1(300, 300);
        Point cp1(400, 400);
        Point cp2(500, 500);
        Point p2(600, 600);
        ASSERT_EQ(index, 5u);
        ASSERT_EQ(cubic.p1, p1);
        ASSERT_EQ(cubic.cp1, cp1);
        ASSERT_EQ(cubic.cp2, cp2);
        ASSERT_EQ(cubic.p2, p2);
      },
      [](size_t index, const ContourComponent& contour) {
        // There is an initial countour added for each curve.
        if (index == 0u) {
          Point p1(0, 0);
          ASSERT_EQ(contour.destination, p1);
        } else if (index == 2u) {
          Point p1(100, 100);
          ASSERT_EQ(contour.destination, p1);
        } else if (index == 4u) {
          Point p1(300, 300);
          ASSERT_EQ(contour.destination, p1);
        } else {
          ASSERT_FALSE(true);
        }
        ASSERT_FALSE(contour.is_closed);
      });
}

}  // namespace testing
}  // namespace impeller
