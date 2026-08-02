#pragma once
#include <vector>
#include <av_s/av_storage.hpp>
#include <av_root/av_request.hpp>

namespace avS
{
    /// @brief Storage for environments and their variables. One class, two tables:
    ///        `environments` (parent) and `environment_variables` (child, FK cascade).
    class AvEnvironmentStorage : private AvStorage
    {
    public:
        AvEnvironmentStorage();
        ~AvEnvironmentStorage();

        using AvStorage::get_db_path;

        /// @brief Upserts the environment row, then every variable it owns.
        ///        Does not open a transaction itself (see the vector overload).
        void upsert(avR::AvEnvironment &environment) const;
        void upsert(std::vector<avR::AvEnvironment> &environments) const;

        void upsert_var(avR::AvEnvironmentVariable &variable) const;
        void upsert_vars(std::vector<avR::AvEnvironmentVariable> &variables) const;

        /// @brief All environments, each with its `vars` filled in.
        std::vector<avR::AvEnvironment> select_all() const;
        std::vector<avR::AvEnvironmentVariable> select_vars_by_env_id(int64_t envId) const;

        /// @brief Deletes the environment; its variables go with it (ON DELETE CASCADE).
        void del(int64_t id) const;
        void del_var(int64_t id) const;
        void del_vars_by_env_id(int64_t envId) const;

    private:
        const uint_fast8_t ecol_id = 1;
        const uint_fast8_t ecol_name = 2;

        const uint_fast8_t vcol_id = 1;
        const uint_fast8_t vcol_env_id = 2;
        const uint_fast8_t vcol_key = 3;
        const uint_fast8_t vcol_value = 4;

        const char *create_environments_table_sql =
            "CREATE TABLE IF NOT EXISTS environments("
            "id INTEGER PRIMARY KEY,"
            "name TEXT NOT NULL"
            ");";

        const char *create_environment_variables_table_sql =
            "CREATE TABLE IF NOT EXISTS environment_variables("
            "id INTEGER PRIMARY KEY,"
            "env_id INTEGER NOT NULL,"
            "vkey TEXT,"
            "vvalue TEXT,"
            "FOREIGN KEY(env_id) REFERENCES environments(id) ON DELETE CASCADE"
            ");";

        const char *create_environment_variables_index_sql =
            "CREATE INDEX IF NOT EXISTS ix_environment_variables_env_id "
            "ON environment_variables(env_id);";

        const char *upsert_environment_sql = "INSERT INTO environments "
                                             "(id, name) "
                                             "VALUES (?,?) "
                                             "ON CONFLICT(id) DO UPDATE SET "
                                             "name=excluded.name;";

        const char *upsert_environment_variable_sql = "INSERT INTO environment_variables "
                                                      "(id, env_id, vkey, vvalue) "
                                                      "VALUES (?,?,?,?) "
                                                      "ON CONFLICT(id) DO UPDATE SET "
                                                      "env_id=excluded.env_id,"
                                                      "vkey=excluded.vkey,"
                                                      "vvalue=excluded.vvalue;";

        const char *select_all_environments_sql = "SELECT * FROM environments ORDER BY id;";
        const char *select_vars_by_env_id_sql =
            "SELECT * FROM environment_variables WHERE env_id = ? ORDER BY id;";

        const char *delete_environment_sql = "DELETE FROM environments WHERE id = ?;";
        const char *delete_environment_variable_sql = "DELETE FROM environment_variables WHERE id = ?;";
        const char *delete_vars_by_env_id_sql = "DELETE FROM environment_variables WHERE env_id = ?;";
    };
} // namespace avS
