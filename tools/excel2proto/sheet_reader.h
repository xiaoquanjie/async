#pragma once

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <fstream>

template<typename KEY1 = int, typename KEY2 = int, typename KEY3 = int, typename KEY4 = int>
struct SheetKey {
public:
    typedef KEY1 _KEY1;
    typedef KEY2 _KEY2;
    typedef KEY3 _KEY3;
    typedef KEY4 _KEY4;

    SheetKey() {
        dataConstruct(this->key1);
        dataConstruct(this->key2);
        dataConstruct(this->key3);
        dataConstruct(this->key4);
    }

    SheetKey(const KEY1& key1) {
        this->key1 = key1;
        dataConstruct(this->key2);
        dataConstruct(this->key3);
        dataConstruct(this->key4);
    }

    SheetKey(const KEY1& key1, const KEY2& key2) {
        this->key1 = key1;
        this->key2 = key2;
        dataConstruct(this->key3);
        dataConstruct(this->key4);
    }

    SheetKey(const KEY1& key1, const KEY2& key2, const KEY3& key3) {
        this->key1 = key1;
        this->key2 = key2;
        this->key3 = key3;
        dataConstruct(this->key4);
    }

    SheetKey(const KEY1& key1, const KEY2& key2, const KEY3& key3, const KEY4& key4) {
        this->key1 = key1;
        this->key2 = key2;
        this->key3 = key3;
        this->key4 = key4;
    }

    template<typename T>
    void dataConstruct(T& v) {
        if (typeid(v) != typeid(std::string)) {
		    memset(&v, 0, sizeof(v));
	    }
    } 

    bool operator<(const SheetKey<KEY1, KEY2, KEY3, KEY4>& other) const {
        if (this->key1 < other.key1)
        {
            return true;
        }
        if (this->key1 > other.key1)
        {
            return false;
        }

        if (this->key2 < other.key2)
        {
            return true;
        }
        if (this->key2 > other.key2)
        {
            return false;
        }

        if (this->key3 < other.key3)
        {
            return true;
        }
        if (this->key3 > other.key3)
        {
            return false;
        }

        if (this->key4 < other.key4)
        {
            return true;
        }
        if (this->key4 > other.key4)
        {
            return false;
        }

        return false;
    }

    KEY1 key1;
    KEY2 key2;
    KEY3 key3;
    KEY4 key4;
};

template<typename KEY, typename NEW_ITEM_TYPE, typename ITEM_TYPE, typename ITEM_ARRAY_TYPE>
struct SheetReader {
public:    
    virtual bool Load(const char* file_path) {
        std::ifstream ifs(file_path, std::ifstream::in);
        if (!ifs) {
            printf("failed to open proto file:%s\n", file_path);
            assert(false);
            return false;
        }

        mArray = std::make_shared<ITEM_ARRAY_TYPE>();
        mItemMap = std::make_shared<std::map<KEY, std::shared_ptr<NEW_ITEM_TYPE>>>();

		google::protobuf::io::IstreamInputStream inputStream(&ifs);
		if (!google::protobuf::TextFormat::Parse(&inputStream, mArray.get())) {
			printf("failed to parser proto file:%s\n", file_path);
            assert(false);
            return false;
		}

        for (size_t idx = 0; idx < mArray->items_size(); ++idx) {
            std::shared_ptr<NEW_ITEM_TYPE> ptr = std::make_shared<NEW_ITEM_TYPE>();
            KEY key;
            if (!parser(key, *ptr.get(), *(mArray->mutable_items(idx)))) {
                printf("failed to parser item:%s|%s\n", file_path, mArray->mutable_items(idx)->ShortDebugString().c_str());
                return false;
            }
            (*mItemMap)[key] = ptr;
        }
        return true;
    }

    std::shared_ptr<NEW_ITEM_TYPE> GetItem(const KEY& key) const {
        auto iter = mItemMap->find(key);
        if (iter == mItemMap->end()) {
            return nullptr;
        }

        return iter->second;
    }

    std::shared_ptr<NEW_ITEM_TYPE> GetItem(const typename KEY::_KEY1& key1) const {
        return GetItem(SheetKey<typename KEY::_KEY1>(key1));
    }

    std::shared_ptr<NEW_ITEM_TYPE> GetItem(const typename KEY::_KEY1& key1, const typename KEY::_KEY2& key2) const {
        return GetItem(SheetKey<typename KEY::_KEY1, typename KEY::_KEY2>(key1, key2));
    }

    std::shared_ptr<NEW_ITEM_TYPE> GetItem(const typename KEY::_KEY1& key1, const typename KEY::_KEY2& key2, const typename KEY::_KEY3& key3) {
        return GetItem(SheetKey<typename KEY::_KEY1, typename KEY::_KEY2, typename KEY::_KEY3>(key1, key2, key3));
    }

    std::shared_ptr<NEW_ITEM_TYPE> GetItem(const typename KEY::_KEY1& key1, const typename KEY::_KEY2& key2, const typename KEY::_KEY3& key3, const typename KEY::_KEY4& key4) {
        return GetItem(SheetKey<typename KEY::_KEY1, typename KEY::_KEY2, typename KEY::_KEY3, typename KEY::_KEY4>(key1, key2, key3, key4));
    }

    std::shared_ptr<ITEM_ARRAY_TYPE> GetItemArray() const {
        return mArray;
    }

    std::shared_ptr<std::map<KEY, std::shared_ptr<NEW_ITEM_TYPE>>> GetItemMap() const {
        return mItemMap;
    }

    virtual bool check() const { return true; }

protected:
    // 解析器实现
    virtual bool parser(KEY& key, NEW_ITEM_TYPE& new_item, ITEM_TYPE& item) = 0;

protected:
    std::shared_ptr<std::map<KEY, std::shared_ptr<NEW_ITEM_TYPE>>> mItemMap;
    std::shared_ptr<ITEM_ARRAY_TYPE> mArray; 
};