#ifndef CASSANDRA_ITERATOR_H
#define CASSANDRA_ITERATOR_H

#include <cassandra.h>
#include <memory>
#include <string>
#include <iterator>
#include <cstdint>

namespace cass {

class Row {
public:
    Row() : row_(nullptr) {}

    std::string getString(int col) const {
        const char* s; size_t len;
        cass_value_get_string(cass_row_get_column(row_, col), &s, &len);
        return {s, len};
    }
    std::string getUuid(int col) const {
        CassUuid u; char buf[CASS_UUID_STRING_LENGTH];
        cass_value_get_uuid(cass_row_get_column(row_, col), &u);
        cass_uuid_string(u, buf);
        return buf;
    }
    int64_t getInt64(int col) const {
        cass_int64_t v;
        cass_value_get_int64(cass_row_get_column(row_, col), &v);
        return static_cast<int64_t>(v);
    }

private:
    const CassRow* row_;
    explicit Row(const CassIterator* it) : row_(cass_iterator_get_row(it)) {}
    friend class ResultView;
};

using CassResultPtr = std::shared_ptr<const CassResult>;

class ResultView {
public:
    explicit ResultView(CassResultPtr res = nullptr) noexcept : res_(std::move(res)) {}

    class iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = Row;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const Row*;
        using reference         = const Row&;

        iterator() = default;

        explicit iterator(const CassResultPtr& res) : res_guard_(res) {
            if (!res) return;
            CassIterator* raw = cass_iterator_from_result(res.get());
            if (!raw) return;
            if (cass_iterator_next(raw))
                itr_.reset(raw, cass_iterator_free);
            else
                cass_iterator_free(raw);
        }

        reference  operator* ()  const noexcept { row_ = Row{ itr_.get() }; return row_; }
        pointer    operator->() const noexcept { return &(**this); }

        iterator& operator++() {
            if (itr_ && !cass_iterator_next(itr_.get()))
                itr_.reset();
            return *this;
        }
        iterator operator++(int) { iterator t(*this); ++(*this); return t; }

        friend bool operator==(const iterator& a, const iterator& b) { return a.itr_.get() == b.itr_.get(); }
        friend bool operator!=(const iterator& a, const iterator& b) { return !(a == b); }

    private:
        CassResultPtr res_guard_;                       // продлеваем жизнь результата
        std::shared_ptr<CassIterator> itr_{nullptr, cass_iterator_free};
        mutable Row row_;
    };

    iterator begin() const { return iterator{ res_ }; }
    iterator end()   const { return iterator{}; }
    std::size_t size() const { return res_ ? cass_result_row_count(res_.get()) : 0; }

private:
    CassResultPtr res_;
};

} // namespace cass

#endif // CASSANDRA_ITERATOR_H
