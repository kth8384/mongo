/**
 * Copyright (C) 2018 MongoDB Inc.
 *
 * This program is free software: you can redistribute it and/or  modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, the copyright holders give permission to link the
 * code of portions of this program with the OpenSSL library under certain
 * conditions as described in each individual source file and distribute
 * linked combinations including the program with the OpenSSL library. You
 * must comply with the GNU Affero General Public License in all respects
 * for all of the code used other than as permitted herein. If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so. If you do not
 * wish to do so, delete this exception statement from your version. If you
 * delete this exception statement from all source files in the program,
 * then also delete it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/db/commands.h"
#include "mongo/s/commands/cluster_commands_helpers.h"

namespace mongo {
/**
 * Forwards the testing-only restartCatalog command to all shards.
 */
class ClusterRestartCatalogCmd final : public BasicCommand {
public:
    ClusterRestartCatalogCmd() : BasicCommand("restartCatalog") {}

    Status checkAuthForOperation(OperationContext* opCtx,
                                 const std::string& dbname,
                                 const BSONObj& cmdObj) const final {
        // No auth checks as this is a testing-only command.
        return Status::OK();
    }

    bool adminOnly() const final {
        return true;
    }

    bool maintenanceMode() const final {
        return true;
    }

    bool maintenanceOk() const final {
        return false;
    }

    AllowedOnSecondary secondaryAllowed() const final {
        return AllowedOnSecondary::kAlways;
    }

    bool supportsWriteConcern(const BSONObj& cmd) const final {
        return false;
    }

    std::string help() const final {
        return "restart catalog\n"
               "Internal command for testing only. Forwards the restartCatalog command to\n"
               "all shards.\n";
    }

    bool run(OperationContext* opCtx,
             const std::string& db,
             const BSONObj& cmdObj,
             BSONObjBuilder& result) final {
        // This command doesn't operate on a collection namespace, so just pass in an empty
        // NamespaceString.
        const auto namespaceStringForCommand = boost::none;
        auto shardResponses = scatterGatherUnversionedTargetAllShards(
            opCtx,
            db,
            namespaceStringForCommand,
            CommandHelpers::filterCommandRequestForPassthrough(cmdObj),
            ReadPreferenceSetting::get(opCtx),
            Shard::RetryPolicy::kIdempotent);

        std::string errmsg;
        appendRawResponses(opCtx, &errmsg, &result, shardResponses);

        // Intentionally not adding the error message to 'result', as it will already contain all
        // the errors from the shards in a field named 'raw'.
        return errmsg.length() > 0 ? false : true;
    }
};

MONGO_INITIALIZER(RegisterClusterRestartCatalogCommand)(InitializerContext* ctx) {
    if (Command::testCommandsEnabled) {
        // Leaked intentionally: a Command registers itself when constructed.
        new ClusterRestartCatalogCmd();
    }
    return Status::OK();
}
}  // namespace mongo
