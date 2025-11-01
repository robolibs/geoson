#pragma once

#include "concord/concord.hpp"
#include <arpa/inet.h>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <netinet/in.h>

#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace geoget {

    struct Point {
        double lat;
        double lon;
    };

    class PolygonDrawer {
      private:
        int server_fd;
        std::vector<Point> points;
        std::vector<std::vector<Point>> all_polygons;
        std::vector<Point> all_single_points;
        bool is_done;
        std::mutex done_mutex;
        std::condition_variable done_cv;
        bool single_point_mode;
        concord::Datum datum;
        bool select_point;

        std::string get_html();
        std::string get_polygon_html();
        std::string get_single_point_html();
        void handle_request(int client_fd);
        std::string add_point(const std::string &request);
        std::string set_point(const std::string &request);
        std::string new_polygon();
        std::string new_point();
        std::string clear_current_only();
        std::string clear_points();
        std::string handle_done();
        std::vector<Point> collect_points();
        Point collect_single_point();
        void set_datum_from_point(const Point &point);

      public:
        PolygonDrawer(bool select_point = false);
        PolygonDrawer(const concord::Datum &d);
        ~PolygonDrawer();

        bool start(int port = 8080);
        void stop();
        concord::Datum add_datum();
        concord::Datum get_datum();
        const std::vector<std::vector<Point>> &get_all_polygons();
        const std::vector<Point> &get_all_points();
        std::vector<concord::Polygon> get_polygons();
        std::vector<concord::Point> get_points();
    };

} // namespace geoget
