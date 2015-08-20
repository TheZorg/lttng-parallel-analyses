/* Copyright (c) 2014 Fabien Reumont-Locke <fabien.reumont-locke@polymtl.ca>
 *
 * This file is part of lttng-parallel-analyses.
 *
 * lttng-parallel-analyses is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * lttng-parallel-analyses is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with lttng-parallel-analyses.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRACEWRAPPER_H
#define TRACEWRAPPER_H

#include <string>

// Forward declarations
struct bt_context;

/*!
 * \brief The TraceWrapper class is used to lazily initialize
 * contexts and add traces when needed.
 */
class Trace {
public:

    Trace(std::string tracePath);

    // Copying isn't allowed
    Trace(const Trace &other) = delete;

    // Moving is ok though (C++11)
    Trace(Trace &&other);
    Trace &operator=(Trace &&other);

    ~Trace();

    /*!
     * \brief Initialize the context if not initialized
     * and return it.
     * \return The context.
     */
    bt_context *getContext();

private:
    bt_context *ctx;
    std::string tracePath;
};

#endif // TRACEWRAPPER_H
