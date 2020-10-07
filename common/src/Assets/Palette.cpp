/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Palette.h"

#include "Ensure.h"
#include "Exceptions.h"
#include "IO/File.h"
#include "IO/Reader.h"
#include "IO/FileSystem.h"
#include "IO/ImageLoader.h"

#include <kdl/string_format.h>

namespace TrenchBroom {
    namespace Assets {
        struct PaletteData {
            /**
             * 1024 bytes, RGBA order.
             */
            std::vector<unsigned char> opaqueData;
            /**
             * 1024 bytes, RGBA order.
             */
            std::vector<unsigned char> index255TransparentData;
        };

        static std::shared_ptr<PaletteData> makeData(std::vector<unsigned char> data) {
            ensure(data.size() == 768, "expected 768 bytes");

            PaletteData result;
            result.opaqueData.reserve(1024);

            for (size_t i = 0; i < 256; ++i) {
                const auto r = data[3 * i + 0];
                const auto g = data[3 * i + 1];
                const auto b = data[3 * i + 2];

                result.opaqueData.push_back(r);
                result.opaqueData.push_back(g);
                result.opaqueData.push_back(b);
                result.opaqueData.push_back(0xFF);
            }

            // build index255TransparentData from opaqueData
            result.index255TransparentData = result.opaqueData;
            result.index255TransparentData[1023] = 0;

            return std::make_shared<PaletteData>(std::move(result));
        }


        Palette::Palette() {}

        Palette::Palette(std::vector<unsigned char> data) :
        m_data(makeData(data)) {}

        Palette Palette::loadFile(const IO::FileSystem& fs, const IO::Path& path) {
            try {
                auto file = fs.openFile(path);
                auto reader = file->reader().buffer();
                const auto extension = kdl::str_to_lower(path.extension());
                if (extension == "lmp") {
                    return loadLmp(reader);
                } else if (extension == "pcx") {
                    return loadPcx(reader);
                } else if (extension == "bmp") {
                    return loadBmp(reader);
                } else {
                    throw AssetException("Could not load palette file '" + path.asString() + "': Unknown palette format");
                }
            } catch (const FileSystemException& e) {
                throw AssetException("Could not load palette file '" + path.asString() + "': " + e.what());
            }
        }

        Palette Palette::loadLmp(IO::Reader& reader) {
            auto data = std::vector<unsigned char>(reader.size());
            reader.read(data.data(), data.size());
            return Palette(std::move(data));
        }

        Palette Palette::loadPcx(IO::Reader& reader) {
            auto data = std::vector<unsigned char>(768);
            reader.seekFromEnd(data.size());
            reader.read(data.data(), data.size());
            return Palette(std::move(data));
        }

        Palette Palette::loadBmp(IO::Reader& reader) {
            auto bufferedReader = reader.buffer();
            IO::ImageLoader imageLoader(IO::ImageLoader::BMP, std::begin(bufferedReader), std::end(bufferedReader));
            auto data = imageLoader.hasPalette() ? imageLoader.loadPalette() : imageLoader.loadPixels(IO::ImageLoader::RGB);
            return Palette(std::move(data));
        }

        Palette Palette::fromRaw(IO::Reader& reader) {
            auto data = std::vector<unsigned char>(reader.size());
            reader.read(data.data(), data.size());
            return Palette(std::move(data));
        }

        bool Palette::initialized() const {
            return m_data.get() != nullptr;
        }

        bool Palette::indexedToRgba(IO::BufferedReader& reader, const size_t pixelCount, std::vector<unsigned char>& rgbaImage, const PaletteTransparency transparency, Color& averageColor) const {
            const unsigned char* paletteData =
                (transparency == PaletteTransparency::Opaque)
                ? m_data->opaqueData.data()
                : m_data->index255TransparentData.data();

            assert(reader.canRead(pixelCount));

            const unsigned char *indexedImage = reinterpret_cast<const unsigned char*>(reader.begin() + reader.position());
            reader.seekForward(pixelCount);

            // Write rgba pixels
            unsigned char* dest = rgbaImage.data();
            for (size_t i = 0; i < pixelCount; ++i) {
                const int index = static_cast<int>(indexedImage[i]);

                memcpy(dest + (i * 4), &paletteData[index * 4], 4);
            }

            // Check average color
            uint32_t colorSum[3] = {0, 0, 0};
            for (size_t i = 0; i < pixelCount; ++i) {
                colorSum[0] += static_cast<uint32_t>(dest[(i * 4) + 0]);
                colorSum[1] += static_cast<uint32_t>(dest[(i * 4) + 1]);
                colorSum[2] += static_cast<uint32_t>(dest[(i * 4) + 2]);
            }
            averageColor = Color(colorSum[0] / (255.0f * pixelCount),
                                 colorSum[1] / (255.0f * pixelCount),
                                 colorSum[2] / (255.0f * pixelCount),
                                 1.0f);

            // Check for transparency
            bool hasTransparency = false;
            if (transparency == PaletteTransparency::Index255Transparent) {
                // Take the bitwise AND of the alpha channel of all pixels
                unsigned char andAlpha = 0xff;
                for (size_t i = 0; i < pixelCount; ++i) {
                    andAlpha &= dest[(i * 4) + 3];
                }
                hasTransparency = (andAlpha != 0xff);
            }

            return hasTransparency;
        }
    }
}
