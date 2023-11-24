#pragma once

#include <algorithm>
#include <ios>
#include <limits>
#include <optional>

#include <Eigen/Core>

namespace fort {
namespace gl {

template <typename T> class RectanglePacker {
public:
	using Point = Eigen::Vector<T, 2>;

	struct Placement {
		Point Position;
		bool  Rotated;
	};

	inline RectanglePacker(T width, T height, bool inhibitRotate = false)
	    : d_size{width, height}
	    , d_noRotate{inhibitRotate} {
		d_skyline.push_back(Point::Zero());
		d_skyline.push_back(Point{0, d_size.y()});
	}

	inline RectanglePacker(const Point &size, bool inhibitRotate = false)
	    : d_size{size}
	    , d_noRotate{inhibitRotate} {
		d_skyline.push_back(Point::Zero());
		d_skyline.push_back(Point{0, d_size.y()});
	}

	inline std::optional<Placement> Place(const Point &size) {
		// std::cerr << "Placing (" << size.transpose() << ") skyline: [ ";

		// for (const auto &p : d_skyline) {
		// 	std::cerr << "(" << p.transpose() << ") ";
		// }
		// std::cerr << "]" << std::endl;

		auto fit = Fit(size);
		auto fitRotated =
		    d_noRotate ? d_skyline.size() : Fit(Point{size.y(), size.x()});
		bool rotated = false;

		// std::cerr << "normal fit: " << std::boolalpha
		//           << (fit < d_skyline.size()) << "( " << fit << " )"
		//           << " rotated fit: " << (fitRotated < d_skyline.size())
		//           << " ( " << fitRotated << " ) " << std::endl;

		if (fit >= d_skyline.size() - 1) {
			if (fitRotated >= d_skyline.size() - 1) {
				// None found
				return std::nullopt;
			}
			// Only rotated found
			fit     = fitRotated;
			rotated = true;
		} else if (fitRotated < d_skyline.size() - 1) {
			T right        = d_skyline[fit].x() + size.x();
			T rightRotated = d_skyline[fitRotated].x() + size.y();
			if (rightRotated < right) {
				// rotated is a better fit.
				rotated = true;
				fit     = fitRotated;
			}
		}

		Point topLeft     = d_skyline[fit];
		Point bottomRight = topLeft + size;
		if (rotated) {
			bottomRight = topLeft + Point{size.y(), size.x()};
		}
		Point topRight = bottomRight;
		topRight.y()   = topLeft.y();

		Point bottomLeft = bottomRight;

		auto next = std::find_if(
		    d_skyline.begin(),
		    d_skyline.end(),
		    [&bottomRight](const auto &p) -> bool {
			    return p.y() >= bottomRight.y();
		    }
		);
		// std::cerr << "next at: " << std::distance(d_skyline.begin(), next)
		//           << "next: " << next->transpose() << std::endl;

		bottomLeft.x() = next->x();

		// std::cerr << "topLeft: " << topLeft.transpose() << std::endl;
		// std::cerr << "topRight: " << topRight.transpose() << std::endl;
		// std::cerr << "bottomRight: " << bottomRight.transpose() << std::endl;
		// std::cerr << "bottomLeft: " << bottomLeft.transpose() << std::endl;

		d_skyline[fit] = topRight;
		d_skyline.insert(
		    d_skyline.begin() + fit + 1,
		    {bottomRight, bottomLeft}
		);

		RebuildSkyline();

		return Placement{
		    .Position = topLeft,
		    .Rotated  = rotated,
		};
	}

private:
	std::vector<Point> d_skyline;
	Point              d_size;
	bool               d_noRotate;

	inline size_t Fit(const Point &size) {
		T      right = d_size.y() + 1;
		size_t idx   = d_skyline.size();

		for (size_t i = 0; i < d_skyline.size(); i++) {
			Point end = d_skyline[i] + size;
			bool  fit = ((end - d_size).array() <= 0).all();
			if (fit == false) {
				// early exit
				continue;
			}

			// test now if another skyline point make it unfit
			for (size_t j = i + 1; j < d_skyline.size(); ++j) {
				if (d_skyline[j].y() >= end.y()) {
					// no more point to test
					break;
				}
				if (d_skyline[j].x() > d_skyline[i].x()) {
					// cannot fit it
					fit = false;
					break;
				}
			}

			if (fit == false) {
				continue;
			}

			if (end.x() < right) {
				right = end.x();
				idx   = i;
			}
		}
		return idx;
	}

	inline void RebuildSkyline() {
		std::vector<Point> skyline;

		skyline.reserve(d_skyline.size());
		skyline.push_back(d_skyline.front());

		for (size_t i = 1; i < d_skyline.size(); ++i) {
			auto &last = skyline.back();
			auto &p    = d_skyline[i];

			bool goDown = skyline.size() % 2 == 1;
			if (goDown == true) {
				if (p.x() == last.x()) {
					if (last.y() >= p.y()) {
						// discard the point
						continue;
					}
					skyline.push_back(p);
				} else if (last.y() == p.y()) {
					// skip previous point and continue
					skyline.back() = p;
				}
			} else {
				if (p.x() == last.x()) {
					skyline.back() = p;
				} else if (p.y() == last.y()) {
					skyline.push_back(p);
				}
			}
		}
		std::swap(d_skyline, skyline);
	}
};

} // namespace gl
} // namespace fort
