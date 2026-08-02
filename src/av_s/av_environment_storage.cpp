#include <av_s/av_environment_storage.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

namespace avS
{
    AvEnvironmentStorage::AvEnvironmentStorage() : AvStorage()
    {
        this->db->exec(this->create_environments_table_sql);
        this->db->exec(this->create_environment_variables_table_sql);
        this->db->exec(this->create_environment_variables_index_sql);
    }

    AvEnvironmentStorage::~AvEnvironmentStorage()
    {
    }

    void AvEnvironmentStorage::upsert(avR::AvEnvironment &environment) const
    {
        SQLite::Statement q(*this->db, this->upsert_environment_sql);
        bool isInsert = environment.id == 0;
        if (isInsert)
            q.bind(this->ecol_id);
        else
            q.bind(this->ecol_id, environment.id);

        q.bind(this->ecol_name, environment.name);
        q.exec();

        if (isInsert)
            environment.id = this->db->getLastInsertRowid();

        for (avR::AvEnvironmentVariable &var : environment.vars)
        {
            var.EnvId = environment.id;
            this->upsert_var(var);
        }
    }

    void AvEnvironmentStorage::upsert(std::vector<avR::AvEnvironment> &environments) const
    {
        SQLite::Transaction tran(*this->db.get());
        for (avR::AvEnvironment &env : environments)
            this->upsert(env);
        tran.commit();
    }

    void AvEnvironmentStorage::upsert_var(avR::AvEnvironmentVariable &variable) const
    {
        SQLite::Statement q(*this->db, this->upsert_environment_variable_sql);
        bool isInsert = variable.id == 0;
        if (isInsert)
            q.bind(this->vcol_id);
        else
            q.bind(this->vcol_id, variable.id);

        q.bind(this->vcol_env_id, variable.EnvId);
        q.bind(this->vcol_key, variable.key);
        q.bind(this->vcol_value, variable.value);
        q.exec();

        if (isInsert)
            variable.id = this->db->getLastInsertRowid();
    }

    void AvEnvironmentStorage::upsert_vars(std::vector<avR::AvEnvironmentVariable> &variables) const
    {
        SQLite::Transaction tran(*this->db.get());
        for (avR::AvEnvironmentVariable &var : variables)
            this->upsert_var(var);
        tran.commit();
    }

    std::vector<avR::AvEnvironment> AvEnvironmentStorage::select_all() const
    {
        std::vector<avR::AvEnvironment> res;
        SQLite::Statement q(*this->db, this->select_all_environments_sql);
        while (q.executeStep())
        {
            avR::AvEnvironment e;
            e.id = q.getColumn(this->ecol_id - 1).getInt64();
            e.name = q.getColumn(this->ecol_name - 1).getText();
            res.push_back(std::move(e));
        }

        for (avR::AvEnvironment &e : res)
            e.vars = this->select_vars_by_env_id(e.id);

        return res;
    }

    std::vector<avR::AvEnvironmentVariable> AvEnvironmentStorage::select_vars_by_env_id(int64_t envId) const
    {
        std::vector<avR::AvEnvironmentVariable> res;
        SQLite::Statement q(*this->db, this->select_vars_by_env_id_sql);
        q.bind(1, envId);
        while (q.executeStep())
        {
            avR::AvEnvironmentVariable v;
            v.id = q.getColumn(this->vcol_id - 1).getInt64();
            v.EnvId = q.getColumn(this->vcol_env_id - 1).getInt64();
            v.key = q.getColumn(this->vcol_key - 1).getText();
            v.value = q.getColumn(this->vcol_value - 1).getText();
            res.push_back(std::move(v));
        }
        return res;
    }

    void AvEnvironmentStorage::del(int64_t id) const
    {
        SQLite::Statement q(*this->db, this->delete_environment_sql);
        q.bind(1, id);
        q.exec();
    }

    void AvEnvironmentStorage::del_var(int64_t id) const
    {
        SQLite::Statement q(*this->db, this->delete_environment_variable_sql);
        q.bind(1, id);
        q.exec();
    }

    void AvEnvironmentStorage::del_vars_by_env_id(int64_t envId) const
    {
        SQLite::Statement q(*this->db, this->delete_vars_by_env_id_sql);
        q.bind(1, envId);
        q.exec();
    }
} // namespace avS
