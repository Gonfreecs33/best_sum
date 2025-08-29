#ifndef PIECEWISE_HPP
#define PIECEWISE_HPP

#include <memory>
#include <string>
#include <fstream>
#include <cmath>
namespace list_version {

struct Segment {
    double x_left, y_left;
    double x_right, y_right;
    std::shared_ptr<Segment> next = nullptr;

    Segment(double xl, double yl, double xr, double yr)
        : x_left(xl), y_left(yl), x_right(xr), y_right(yr) {}

    double get_slope() const {
        return (y_right - y_left) / (x_right - x_left);
    }

    double evaluate(double x) const {
        double slope = get_slope();
        return y_left + slope * (x - x_left);
    }
};

class PiecewiseLinearFunction {
public:
    std::shared_ptr<Segment> head = nullptr;

    void add_segment(std::shared_ptr<Segment> seg) {
        if (!head) {
            head = seg;
        } else {
            auto current = head;
            while (current->next) current = current->next;
            current->next = seg;
        }
    }

    void export_to_csv(const std::string& filename) const {
        std::ofstream file(filename);
        auto current = head;
        while (current) {
            file << current->x_left << "," << current->y_left << "\n";
            file << current->x_right << "," << current->y_right << "\n";
            current = current->next;
        }
        file.close();
    }

    void simplify() {
        if (!head || !head->next) return;

        auto current = head;
        while (current->next) {
            auto next = current->next;
            double slope1 = current->get_slope();
            double slope2 = next->get_slope();

            if (std::abs(slope1 - slope2) < 1e-9 && std::abs(current->y_right - next->y_left) < 1e-9) {
                // Fusion
                current->x_right = next->x_right;
                current->y_right = next->y_right;
                current->next = next->next;
            } else {
                current = current->next;
            }
        }
    }

    void add(const PiecewiseLinearFunction& other) ;

};



//=====================================================================================================================
//=====================================================================================================================
//===================================   Build delta and task contribution profile    ==================================
//=====================================================================================================================

PiecewiseLinearFunction delta_profile_temp(double gap, double a, double b , double c, double horizon ){

    PiecewiseLinearFunction delta;
    auto seg1 = std::make_shared<Segment>(0, 0.0, a, 0.0);
    auto seg2 = std::make_shared<Segment>(a, 0.0, b, gap);
    auto seg3 = std::make_shared<Segment>(b, gap, c, 0.0);
    auto seg4 = std::make_shared<Segment>(c, 0.0, horizon, 0.0);
    delta.add_segment(seg1);
    delta.add_segment(seg2);
    delta.add_segment(seg3);
    delta.add_segment(seg4);

    return delta ;

}

PiecewiseLinearFunction cba_profile(double cap, double a, double b, double horizon ) {

    PiecewiseLinearFunction cba;

            auto seg1= std::make_shared<Segment>(0, 0, a, 0);
            auto seg2 = std::make_shared<Segment>(a, 0 , b , cap);
            auto seg3 = std::make_shared<Segment>(b, cap, horizon, cap);
            cba.add_segment(seg1);
            cba.add_segment(seg2);
            cba.add_segment(seg3);  

    return cba;
}

//=====================================================================================================================
//=====================================================================================================================
//============================================ Elementary operation (sum, min max...)==================================
//=====================================================================================================================

PiecewiseLinearFunction add_functions(const PiecewiseLinearFunction& f1, const PiecewiseLinearFunction& f2) {
    PiecewiseLinearFunction result;
    auto a = f1.head;
    auto b = f2.head;

    while (a && b) {
        double start = std::max(a->x_left, b->x_left);
        double end = std::min(a->x_right, b->x_right);

        if (start > end) {
            if (a->x_right < b->x_right) a = a->next;
            else b = b->next;
            continue;
        }

        double y1_start = a->evaluate(start);
        double y1_end = a->evaluate(end);
        double y2_start = b->evaluate(start);
        double y2_end = b->evaluate(end);

        auto seg = std::make_shared<Segment>(start, y1_start + y2_start, end, y1_end + y2_end);
        result.add_segment(seg);

        if (a->x_right <= end) a = a->next;
        if (b->x_right <= end) b = b->next;
    }
    return result;
}

void PiecewiseLinearFunction::add(const PiecewiseLinearFunction& other) {
    PiecewiseLinearFunction sum = add_functions(*this, other);
    head = sum.head;
}






PiecewiseLinearFunction negate(const PiecewiseLinearFunction& f) {
    PiecewiseLinearFunction result;
    auto current = f.head;
    while (current) {
        result.add_segment(std::make_shared<Segment>(
            current->x_left, -current->y_left,
            current->x_right, -current->y_right
        ));
        current = current->next;
    }
    return result;
}

// PiecewiseLinearFunction min_functions(const PiecewiseLinearFunction& f1, const PiecewiseLinearFunction& f2) {
//     PiecewiseLinearFunction result;
//     auto a = f1.head;
//     auto b = f2.head;

//     while (a && b) {
//         double start = std::max(a->x_left, b->x_left);
//         double end = std::min(a->x_right, b->x_right);

//         if (start > end) {
//             if (a->x_right < b->x_right) a = a->next;
//             else b = b->next;
//             continue;
//         }

//         double fa_start = a->evaluate(start);
//         double fa_end = a->evaluate(end);
//         double fb_start = b->evaluate(start);
//         double fb_end = b->evaluate(end);

//         bool intersect = (fa_start - fb_start) * (fa_end - fb_end) < 0;

//         if (!intersect) {
//             auto chosen = ((fa_start <= fb_start) && (fa_end <= fb_end)) ? a : b;
//             result.add_segment(std::make_shared<Segment>(
//                 start, chosen->evaluate(start),
//                 end, chosen->evaluate(end)
//             ));
//         }
//         else {
//             double m1 = a->get_slope();
//             double m2 = b->get_slope();
//             double c1 = a->y_left - m1 * a->x_left;
//             double c2 = b->y_left - m2 * b->x_left;
//             double x_star = (c2 - c1) / (m1 - m2);

//             auto chosen1 = (a->evaluate((start + x_star) / 2.0) < b->evaluate((start + x_star) / 2.0)) ? a : b;
//             result.add_segment(std::make_shared<Segment>(
//                 start, chosen1->evaluate(start),
//                 x_star, chosen1->evaluate(x_star)
//             ));

//             auto chosen2 = (a->evaluate((x_star + end) / 2.0) < b->evaluate((x_star + end) / 2.0)) ? a : b;
//             result.add_segment(std::make_shared<Segment>(
//                 x_star, chosen2->evaluate(x_star),
//                 end, chosen2->evaluate(end)
//             ));
//         }

//         if (a->x_right <= end) a = a->next;
//         if (b->x_right <= end) b = b->next;
//     }
//     result.simplify();
//     return result;
// }

// PiecewiseLinearFunction max_functions(const PiecewiseLinearFunction& f1, const PiecewiseLinearFunction& f2) {
//     PiecewiseLinearFunction result;
//     auto a = f1.head;
//     auto b = f2.head;

//     while (a && b) {
//         double start = std::max(a->x_left, b->x_left);
//         double end = std::min(a->x_right, b->x_right);

//         if (start > end) {
//             if (a->x_right < b->x_right) a = a->next;
//             else b = b->next;
//             continue;
//         }

//         double fa_start = a->evaluate(start);
//         double fa_end = a->evaluate(end);
//         double fb_start = b->evaluate(start);
//         double fb_end = b->evaluate(end);

//         bool intersect = (fa_start - fb_start) * (fa_end - fb_end) < 0;

//         if (!intersect) {
//             auto chosen = ((fa_start >= fb_start) && (fa_end >= fb_end)) ? a : b;
//             result.add_segment(std::make_shared<Segment>(
//                 start, chosen->evaluate(start),
//                 end, chosen->evaluate(end)
//             ));
//         } else {
//             double m1 = a->get_slope();
//             double m2 = b->get_slope();
//             double c1 = a->y_left - m1 * a->x_left;
//             double c2 = b->y_left - m2 * b->x_left;
//             double x_star = (c2 - c1) / (m1 - m2);

//             auto chosen1 = (a->evaluate((start + x_star) / 2.0) > b->evaluate((start + x_star) / 2.0)) ? a : b;
//             result.add_segment(std::make_shared<Segment>(
//                 start, chosen1->evaluate(start),
//                 x_star, chosen1->evaluate(x_star)
//             ));

//             auto chosen2 = (a->evaluate((x_star + end) / 2.0) > b->evaluate((x_star + end) / 2.0)) ? a : b;
//             result.add_segment(std::make_shared<Segment>(
//                 x_star, chosen2->evaluate(x_star),
//                 end, chosen2->evaluate(end)
//             ));
//         }

//         if (a->x_right <= end) a = a->next;
//         if (b->x_right <= end) b = b->next;
//     }

//     result.simplify();
//     return result;
// }

// PiecewiseLinearFunction min_function_with_constant(const PiecewiseLinearFunction& f, const PiecewiseLinearFunction& g) {
//     PiecewiseLinearFunction result;
//     auto seg = f.head;
//     auto c = g.head;

//     double constant = c->y_left;  // on suppose g est constante

//     while (seg) {
//         double fa_start = seg->evaluate(seg->x_left);
//         double fa_end = seg->evaluate(seg->x_right);
//         bool intersects = (fa_start - constant) * (fa_end - constant) < 0;

//         if (!intersects) {
//             if (fa_start <= constant && fa_end <= constant) {
//                 result.add_segment(std::make_shared<Segment>(
//                     seg->x_left, fa_start,
//                     seg->x_right, fa_end
//                 ));
//             } else {
//                 result.add_segment(std::make_shared<Segment>(
//                     seg->x_left, constant,
//                     seg->x_right, constant
//                 ));
//             }
//         } else {
//             double m = seg->get_slope();
//             double c1 = seg->y_left - m * seg->x_left;
//             double x_star = (constant - c1) / m;

//             if (fa_start > constant) {
//                 result.add_segment(std::make_shared<Segment>(
//                     seg->x_left, constant,
//                     x_star, constant
//                 ));
//             } else {
//                 result.add_segment(std::make_shared<Segment>(
//                     seg->x_left, fa_start,
//                     x_star, constant
//                 ));
//             }

//             if (fa_end > constant) {
//                 result.add_segment(std::make_shared<Segment>(
//                     x_star, constant,
//                     seg->x_right, constant
//                 ));
//             } else {
//                 result.add_segment(std::make_shared<Segment>(
//                     x_star, constant,
//                     seg->x_right, fa_end
//                 ));
//             }
//         }

//         seg = seg->next;
//     }
//     result.simplify();
//     return result;
// }

// PiecewiseLinearFunction max_function_with_constant(const PiecewiseLinearFunction& f, const PiecewiseLinearFunction& g) {
//     PiecewiseLinearFunction result;
//     auto seg = f.head;
//     auto c = g.head;

//     double constant = c->y_left;  // on suppose g est constante

//     while (seg) {
//         double fa_start = seg->evaluate(seg->x_left);
//         double fa_end = seg->evaluate(seg->x_right);
//         bool intersects = (fa_start - constant) * (fa_end - constant) < 0;

//         if (!intersects) {
//             if (fa_start >= constant && fa_end >= constant) {
//                 result.add_segment(std::make_shared<Segment>(
//                     seg->x_left, fa_start,
//                     seg->x_right, fa_end
//                 ));
//             } else {
//                 result.add_segment(std::make_shared<Segment>(
//                     seg->x_left, constant,
//                     seg->x_right, constant
//                 ));
//             }
//         } else {
//             double m = seg->get_slope();
//             double c1 = seg->y_left - m * seg->x_left;
//             double x_star = (constant - c1) / m;

//             if (fa_start > constant) {
//                 result.add_segment(std::make_shared<Segment>(
//                     seg->x_left, fa_start,
//                     x_star, constant
//                 ));
//             } else {
//                 result.add_segment(std::make_shared<Segment>(
//                     seg->x_left, constant,
//                     x_star, constant
//                 ));
//             }

//             if (fa_end > constant) {
//                 result.add_segment(std::make_shared<Segment>(
//                     x_star, constant,
//                     seg->x_right, fa_end
//                 ));
//             } else {
//                 result.add_segment(std::make_shared<Segment>(
//                     x_star, constant,
//                     seg->x_right, constant
//                 ));
//             }
//         }

//         seg = seg->next;
//     }
//     result.simplify();
//     return result;
// }


}

#endif