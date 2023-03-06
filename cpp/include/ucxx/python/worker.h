/** Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
 *
 * See file LICENSE for terms.
 */
#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include <ucp/api/ucp.h>

#include <ucxx/python/future.h>
#include <ucxx/python/notifier.h>
#include <ucxx/worker.h>

namespace ucxx {

namespace python {

class Worker : public ::ucxx::Worker {
 private:
  /**
   * @brief Private constructor of `ucxx::python::Worker`.
   *
   * This is the internal implementation of `ucxx::python::Worker` constructor, made
   * private not to be called directly. Instead the user should call
   * `ucxx::python::createWorker()`.
   *
   * @param[in] context the context from which to create the worker.
   * @param[in] enableDelayedSubmission if `true`, each `ucxx::Request` will not be
   *                                    submitted immediately, but instead delayed to
   *                                    the progress thread. Requires use of the
   *                                    progress thread.
   * @param[in] enableFuture if `true`, notifies the Python future associated with each
   *                         `ucxx::Request`.
   */
  Worker(std::shared_ptr<Context> context,
         const bool enableDelayedSubmission = false,
         const bool enableFuture            = false);

 public:
  Worker()              = delete;
  Worker(const Worker&) = delete;
  Worker& operator=(Worker const&) = delete;
  Worker(Worker&& o)               = delete;
  Worker& operator=(Worker&& o) = delete;

  /**
   * @brief Constructor of `shared_ptr<ucxx::python::Worker>`.
   *
   * The constructor for a `shared_ptr<ucxx::python::Worker>` object. The default
   * constructor is made private to ensure all UCXX objects are shared pointers and correct
   * lifetime management.
   *
   * @code{.cpp}
   * // context is `std::shared_ptr<ucxx::Context>`
   * auto worker = ucxx::createPythonWorker(context, false, false);
   * @endcode
   *
   * @param[in] context the context from which to create the worker.
   * @param[in] enableDelayedSubmission if `true`, each `ucxx::Request` will not be
   *                                    submitted immediately, but instead delayed to
   *                                    the progress thread. Requires use of the
   *                                    progress thread.
   * @param[in] enableFuture if `true`, notifies the Python future associated with each
   *                         `ucxx::Request`.
   * @returns The `shared_ptr<ucxx::python::Worker>` object
   */
  friend std::shared_ptr<::ucxx::Worker> createPythonWorker(std::shared_ptr<Context> context,
                                                            const bool enableDelayedSubmission,
                                                            const bool enableFuture);

  /**
   * @brief Populate the Python future pool.
   *
   * To avoid taking the Python GIL for every new future required by each `ucxx::Request`,
   * the `ucxx::python::Worker` maintains a pool of futures that can be acquired when a new
   * `ucxx::Request` is created. Currently the pool has a maximum size of 100 objects, and
   * will refill once it goes under 50, otherwise calling this functions results in a no-op.
   */
  virtual void populateFuturesPool() override;

  /**
   * @brief Get a Python future from the pool.
   *
   * Get a Python future from the pool. If the pool is empty,
   * `ucxx::python::Worker::populateFuturesPool()` is called and a warning is raised, since
   * that likely means the user is missing to call the aforementioned method regularly.
   *
   * @returns The `shared_ptr<ucxx::python::Future>` object
   */
  virtual std::shared_ptr<::ucxx::Future> getFuture() override;

  /**
   * @brief Block until a request event.
   *
   * Blocks until some communication is completed and a Python future is ready to be
   * notified, shutdown was initiated or a timeout occurred (only if `periodNs > 0`).
   * This method is intended for use from the Python notifier thread, where that
   * thread will block until one of the aforementioned events occur.
   *
   * @returns `RequestNotifierWaitState::Ready` if some communication completed,
   *          `RequestNotifierWaitStats::Timeout` if a timeout occurred, or
   *          `RequestNotifierWaitStats::Shutdown` if shutdown has initiated.
   */
  virtual RequestNotifierWaitState waitRequestNotifier(uint64_t periodNs) override;

  /**
   * @brief Notify Python futures of each completed communication request.
   *
   * Notifies Python futures of each completed communication request of their new status.
   * This method is intended to be used from the Python notifier thread, where the thread
   * will call `waitRequestNotifier()` and block until some communication is completed, and
   * then call this method to notify all futures. If this is notifying a Python future, the
   * thread where this method is called from must be using the same Python event loop as
   * the thread that submitted the transfer request.
   */
  virtual void runRequestNotifier() override;

  /**
   * @brief Signal the notifier to terminate.
   *
   * Signals the notifier to terminate, awakening the `waitRequestNotifier()` blocking call.
   */
  virtual void stopRequestNotifierThread() override;
};

}  // namespace python

}  // namespace ucxx