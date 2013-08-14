/* Copyright (c) 2013 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef RAMCLOUD_ZOOSTORAGE_H
#define RAMCLOUD_ZOOSTORAGE_H

#include "ExternalStorage.h"
#include "WorkerTimer.h"
#include "zookeeper.h"

namespace RAMCloud {

/**
 * This class provides a wrapper around the ZooKeeper storage system to
 * implement the ExternalStorage interface. See ExternalStorage for API
 * documentation.
 */
class ZooStorage: public ExternalStorage {
  PUBLIC:
    explicit ZooStorage(string* serverInfo, Dispatch* dispatch);
    virtual ~ZooStorage();
    virtual void becomeLeader(const char* name, const string* leaderInfo);
    virtual bool get(const char* name, Buffer* value);
    virtual void getChildren(const char* name, vector<Object>* children);
    virtual void remove(const char* name);
    virtual void set(Hint flavor, const char* name, const char* value,
            int valueLength = -1);
    virtual void setLeaderInfo(const string* leaderInfo);

  PRIVATE:
    /**
     * This class is used to update the leader object in order to
     * maintain the leader's legitimacy.
     */
    class LeaseRenewer: public WorkerTimer {
      public:
        explicit LeaseRenewer(ZooStorage* zooStorage);
        virtual ~LeaseRenewer() {}
        virtual void handleTimerEvent();

        /// Copy of constructor argument:
        ZooStorage* zooStorage;

      private:
        DISALLOW_COPY_AND_ASSIGN(LeaseRenewer);
    };

    /// Copy of serverInfo argument from constructor.
    string serverInfo;

    /// Information about the current ZooKeeper connection, or NULL
    /// if none.
    zhandle_t* zoo;

    /// Indicates whether we are currently acting as a valid leader.
    bool leader;

    /// If we are unable to connect to the ZooKeeper server, this variable
    /// determines how often we retry (units: milliseconds).
    int connectionRetryMs;

    /// When a server is waiting to become a leader, it checks the leader
    /// object this often (units: milliseconds); if the object hasn't changed
    /// since the last check, the current server attempts to become leader.
    int checkLeaderIntervalMs;

    /// When a server is leader, it renews its lease this often (units: rdtsc
    /// cycles). This value must be quite a bit less than
    /// checkLeaderIntervalCycles.  A value of zero is used during testing;
    /// it means "don't renew the lease or even start the timer".
    uint64_t renewLeaseIntervalCycles;

    /// If a problem of some sort prevents the lease from being renewed,
    /// the leader tries again this often (units: rdtsc cycles). The goal
    /// is to have enough time to retry several times (after an initial
    /// delay of renewLeaseIntervalCycles) before checkLeaderIntervalCycles
    /// elapses and we lose leadership.
    uint64_t renewLeaseRetryCycles;

    /// This variable holds the most recently seen version number of the
    /// leader object; -1 means that we haven't yet read a version number.
    /// During leader election, this allows us to tell if the current leader
    /// has updated leader object. Once we become leader, it allows us to
    /// detect if we lose leadership.
    int32_t leaderVersion;

    /// Copy of name argument from becomeLeader: name of the object used
    /// for leader election and identification. The current leader renews
    /// its lease by updating this object; if another server detects that
    /// a long period has gone by without the object being updated, then it
    /// claims leadership for itself. The contents of the object will be
    /// set from leaderInfo.
    string leaderObject;

    /// Copy of leaderInfo argument from becomeLeader; empty if becomeLeader
    /// has not been invoked yet.
    string leaderInfo;

    /// Used to update leaderObject.
    Tub<LeaseRenewer> leaseRenewer;

    /// Used for scheduling leaseRenewer.
    Dispatch* dispatch;

    bool checkLeader();
    void close();
    void createParent(const char* childName);
    void handleError(int status);
    void open();
    const char* stateString(int state);
    void updateLeaderObject();

    DISALLOW_COPY_AND_ASSIGN(ZooStorage);
};

} // namespace RAMCloud

#endif // RAMCLOUD_ZOOSTORAGE_H

