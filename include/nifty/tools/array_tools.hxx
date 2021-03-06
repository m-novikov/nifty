#pragma once

#include <algorithm>
#include <vector>

#include "nifty/tools/for_each_coordinate.hxx"
#include "nifty/xtensor/xtensor.hxx"

namespace nifty {
namespace tools {


    // TODO one top function (unique) to select the appropriate unique version

    // unique values in array
    template<class T, class ARRAY>
    inline void uniques(const xt::xexpression<ARRAY> & arrayExp, std::vector<T> & out){
        const auto & array = arrayExp.derived_cast();

        out.resize(array.size());
        // FIXME FIXME FIXME copy is probably slow
        std::copy(array.begin(), array.end(), out.begin());

        std::sort(out.begin(),out.end());
        auto last = std::unique(out.begin(), out.end());
        out.erase( last, out.end() );
    }


    // unique values in vector
    template<class T>
    inline void uniques(const std::vector<T> & array, std::vector<T> & out){
        out.resize(array.size());
        std::copy(array.begin(), array.end(), out.begin());

        std::sort(out.begin(),out.end());
        auto last = std::unique(out.begin(), out.end());
        out.erase( last, out.end() );
    }


    // unique values in masked array
    template<unsigned DIM, class T, class ARRAY, class MASK>
    inline void uniquesWithMask(const xt::xexpression<ARRAY> & arrayExp,
                                const xt::xexpression<MASK> & maskExp,
                                std::vector<T> & out){
        const auto & array = arrayExp.derived_cast();
        const auto & mask = maskExp.derived_cast();

        //TODO check that array and mask have the same size
        typedef array::StaticArray<int64_t, DIM> Coord;
        Coord shape;
        for(int d = 0; d < DIM; ++d) {
            shape[d] = array.shape(d);
        }

        out.clear();

        // copy if, but w.r.t. mask
        forEachCoordinate(shape, [&](const Coord & coord){
            if(xtensor::read(mask, coord.asStdArray())) {
                out.emplace_back( xtensor::read(array, coord.asStdArray()) );
            }
        });

        std::sort(out.begin(), out.end());
        auto last = std::unique(out.begin(), out.end());
        out.erase( last, out.end() );
    }


    // unique values in array from coordinates
    template<unsigned DIM, class T, class ARRAY>
    inline void uniquesWithCoordinates(const xt::xexpression<ARRAY> & arrayExp,
                                       const std::vector<array::StaticArray<int64_t,DIM>> & coordinates,
                                       std::vector<T> & out){
        typedef array::StaticArray<int64_t,DIM> Coord;
        const auto & array = arrayExp.derived_cast();
        out.clear();

        for(auto & coord : coordinates ) {
            out.emplace_back( xtensor::read(array, coord.asStdArray()) );
        }

        std::sort(out.begin(),out.end());
        auto last = std::unique(out.begin(), out.end());
        out.erase( last, out.end() );
    }


    // unique values in masked array from coordinates
    template<unsigned DIM, class T, class ARRAY, class MASK>
    inline void uniquesWithMaskAndCoordinates(const xt::xexpression<ARRAY> & arrayExp,
                                              const xt::xexpression<MASK> & maskExp,
                                              const std::vector<array::StaticArray<int64_t,DIM>> & coordinates,
                                              std::vector<T> & out) {
        //TODO check that array and mask have the same size
        const auto & array = arrayExp.derived_cast();
        const auto & mask = maskExp.derived_cast();

        typedef array::StaticArray<int64_t, DIM> Coord;
        out.clear();

        for(auto & coord : coordinates ) {
            if(xtensor::read(mask, coord.asStdArray())) {
                out.emplace_back( xtensor::read(array, coord.asStdArray()) );
            }
        }

        std::sort(out.begin(),out.end());
        auto last = std::unique(out.begin(), out.end());
        out.erase( last, out.end() );
    }


    // coordinate where array == val
    template<unsigned DIM, class T, class ARRAY>
    inline void
    where(const xt::xexpression<ARRAY> & arrayExp,
          const T val,
          std::vector<array::StaticArray<int64_t,DIM>> & coordsOut) {
        const auto & array = arrayExp.derived_cast();

        NIFTY_CHECK_OP(DIM,==,array.dimension(),"Dimensions do not match!");
        typedef array::StaticArray<int64_t,DIM> Coord;
        coordsOut.clear();

        Coord shape;
        for(int d = 0; d < DIM; ++d)
            shape[d] = array.shape(d);

        // reserve the max size for the vector
        coordsOut.reserve(array.size());

        forEachCoordinate(shape, [&](const Coord & coord){
            if(xtensor::read(array, coord.asStdArray()) == val)
                coordsOut.emplace_back(coord);
        });
    }


    // coordinate where array == val
    // + returns bounding box
    template<unsigned DIM, class T, class ARRAY>
    inline std::pair<array::StaticArray<int64_t, DIM>, array::StaticArray<int64_t, DIM>>
    whereAndBoundingBox(const xt::xexpression<ARRAY> & arrayExp,
                        const T val,
                        std::vector<array::StaticArray<int64_t,DIM>> & coordsOut) {
        const auto & array = arrayExp.derived_cast();

        NIFTY_CHECK_OP(DIM,==,array.dimension(),"Dimensions do not match!");
        typedef array::StaticArray<int64_t,DIM> Coord;
        coordsOut.clear();
        coordsOut.reserve(array.size());

        Coord shape;
        Coord bbBegin; // begin of bounding box
        Coord bbEnd;   // end of bounding box
        for(int d = 0; d < DIM; ++d) {
            shape[d] = array.shape(d);
            bbBegin[d] = array.shape(d);
            bbEnd[d] = 0;
        }

        forEachCoordinate(shape, [&](const Coord & coord){
            if(xtensor::read(array, coord.asStdArray()) == val) {
                coordsOut.emplace_back(coord);
                for(int d = 0; d < DIM; ++ d) {
                    if(coord[d] < bbBegin[d])
                        bbBegin[d] = coord[d];
                    if(coord[d] > bbEnd[d])
                        bbEnd[d] = coord[d];
                }
            }
        });

        // increase end of the bounding box by 1 to have the actual bb coordinates
        for(int d = 0; d < DIM; ++d) {
            ++bbEnd[d];
        }

        return std::make_pair(bbBegin, bbEnd);
    }


    template<unsigned DIM, class T, class ARRAY>
    inline void valuesToCoordinates(const xt::xexpression<ARRAY> & arrayExp,
                                    std::map<T, std::vector<nifty::array::StaticArray<int64_t, DIM>>> & coordMapOut) {
        const auto & array = arrayExp.derived_cast();

        NIFTY_CHECK_OP(DIM,==,array.dimension(),"Dimensions do not match!");
        typedef array::StaticArray<int64_t,DIM> Coord;

        Coord shape;
        for(int d = 0; d < DIM; ++d)
            shape[d] = array.shape(d);

        forEachCoordinate(shape, [&](const Coord & coord){
            T val = xtensor::read(array, coord.asStdArray());
            auto valIt = coordMapOut.find(val);
            if(valIt == coordMapOut.end())
                coordMapOut.emplace(val, std::vector<Coord>({coord}) );
            else
                valIt->second.push_back(coord);
        });
    }


    template<unsigned DIM, class T, class ARRAY>
    inline void valuesToCoordinatesWithCoordinates(const xt::xexpression<ARRAY> & arrayExp,
                                                   const std::vector<array::StaticArray<int64_t,DIM>> & coordinates,
                                                   std::map<T, std::vector<nifty::array::StaticArray<int64_t,DIM>>> & coordMapOut) {
        const auto & array = arrayExp.derived_cast();

        NIFTY_CHECK_OP(DIM,==,array.dimension(),"Dimensions do not match!");
        typedef array::StaticArray<int64_t,DIM> Coord;

        for(const auto & coord : coordinates) {
            T val = xtensor::read(array, coord.asStdArray());
            auto valIt = coordMapOut.find(val);
            if(valIt == coordMapOut.end())
                coordMapOut.emplace(val, std::vector<Coord>({coord}) );
            else
                valIt->second.push_back(coord);
        }
    }


    template<class KEY, class VALUE>
    inline void extractKeys(const std::map<KEY,VALUE> & inMap, std::vector<KEY> & keysOut) {
        keysOut.clear();
        keysOut.reserve(inMap.size());
        std::transform(inMap.begin(), inMap.end(), std::back_inserter(keysOut),
            [&](std::pair<KEY,VALUE> keyVal){return keyVal.first;});
    }


    template<unsigned DIM, class T, class ARRAY>
    inline void mapDictionaryToArray(xt::xexpression<ARRAY> & arrayExp, const std::map<T, T> & dict,
                                     bool haveIgnoreValue=false, T ignoreValue=0) {
        typedef array::StaticArray<int64_t, DIM> Coord;
        auto & array = arrayExp.derived_cast();

        Coord shape;
        for(int i = 0; i < DIM; ++i) {
            shape[i] = array.shape()[i];
        }

        if(haveIgnoreValue) {
            forEachCoordinate(shape, [&](const Coord & coord){
                T val = xtensor::read(array, coord.asStdArray());
                if(val != ignoreValue) {
                    auto mapIt = dict.find(val);
                    if(mapIt != dict.end()) {
                        xtensor::write(array, coord.asStdArray(), mapIt->second);
                    }
                }
            });
        } else {
            forEachCoordinate(shape, [&](const Coord & coord){
                T val = xtensor::read(array, coord.asStdArray());
                auto mapIt = dict.find(val);
                if(mapIt != dict.end()) {
                    xtensor::write(array, coord.asStdArray(), mapIt->second);
                }
            });
        }
    }


} // namespace tools
} // namespace nifty
