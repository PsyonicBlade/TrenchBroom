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

#ifndef TrenchBroom_Palette
#define TrenchBroom_Palette

#include "Color.h"
#include "IO/Reader.h"

#include <cassert>
#include <memory>
#include <vector>

namespace TrenchBroom {
    namespace IO {
        class BufferedReader;
        class FileSystem;
        class Path;
    }

    namespace Assets {
        enum class PaletteTransparency {
            Opaque, Index255Transparent
        };

        struct PaletteData;

        class Palette {
        private:
            std::shared_ptr<PaletteData> m_data;
        public:
            Palette();
            Palette(std::vector<unsigned char> data);

            static Palette loadFile(const IO::FileSystem& fs, const IO::Path& path);
            static Palette loadLmp(IO::Reader& reader);
            static Palette loadPcx(IO::Reader& reader);
            static Palette loadBmp(IO::Reader& reader);
            static Palette fromRaw(IO::Reader& reader);

            bool initialized() const;

            /**
             * Converts the given index buffer to an RGBA image.
             *
             * @param indexedImage the index buffer
             * @param rgbaImage the pixel buffer
             * @param transparency controls whether or not the given index buffer contains a transparent index
             * @param averageColor output parameter for the average color of the generated pixel buffer
             * @return true if the given index buffer did contain a transparent index, unless the transparency parameter
             *     indicates that the image is opaque
             */
            bool indexedToRgba(IO::BufferedReader& reader, size_t pixelCount, std::vector<unsigned char>& rgbaImage, const PaletteTransparency transparency, Color& averageColor) const;
        };
    }
}

#endif /* defined(TrenchBroom_Palette) */
