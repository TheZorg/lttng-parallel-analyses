#ifndef TRACEWRAPPER_H
#define TRACEWRAPPER_H

#include<QString>

// Forward declarations
struct bt_context;

/*!
 * \brief The TraceWrapper class is used to lazily initialize
 * contexts and add traces when needed.
 */
class TraceWrapper {
public:

    TraceWrapper(QString tracePath);

    // Copying isn't allowed
    TraceWrapper(const TraceWrapper &other) = delete;

    // Moving is ok though (C++11)
    TraceWrapper(TraceWrapper &&other);

    TraceWrapper &operator=(TraceWrapper &&other);

    ~TraceWrapper();

    /*!
     * \brief Initialize the context if not initialized
     * and return it.
     * \return The context.
     */
    bt_context *getContext();

private:
    bt_context *ctx;
    QString tracePath;
};

#endif // TRACEWRAPPER_H
