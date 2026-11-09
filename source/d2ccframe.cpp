#include "d2ccframe.h"

#include <QDataStream>

#include "progressdialog.h"

bool D2CcFrame::load(D1GfxFrame &frame, int idx, const QByteArray &rawData, const OpenAsParam &params)
{
    /*
            	const (
		outSizeCodedBits     = 32
		compressionFlagsBits = 2
		variable0Bits        = 4
		widthBits            = 4
		heightBits           = 4
		xOffsetBits          = 4
		yOffsetBits          = 4
		optionalDataBits     = 4
		codedBytesBits       = 4
	)

	var val uint32

	val, _ = stream.Next(outSizeCodedBits).Bits().AsUInt32()
	d.OutSizeCoded = int(val)

	val, _ = stream.Next(compressionFlagsBits).Bits().AsUInt32()
	d.CompressionFlags = int(val)

	d.Variable0Bits, _ = crazyLookup(stream.Next(variable0Bits).Bits().AsUInt32())
	d.WidthBits, _ = crazyLookup(stream.Next(widthBits).Bits().AsUInt32())
	d.HeightBits, _ = crazyLookup(stream.Next(heightBits).Bits().AsUInt32())
	d.XOffsetBits, _ = crazyLookup(stream.Next(xOffsetBits).Bits().AsUInt32())
	d.YOffsetBits, _ = crazyLookup(stream.Next(yOffsetBits).Bits().AsUInt32())
	d.OptionalDataBits, _ = crazyLookup(stream.Next(optionalDataBits).Bits().AsUInt32())
	d.CodedBytesBits, err = crazyLookup(stream.Next(codedBytesBits).Bits().AsUInt32())
    */


    unsigned width = 0;
    // calculate the offset in case of a clipped frame
    int frameDataStartOffset = 0;
    // READ {CEL FRAME DATA}
    std::vector<std::vector<D1GfxPixel>> pixels;
    std::vector<D1GfxPixel> pixelLine;
    for (int o = frameDataStartOffset; o < rawData.size(); o++) {
        quint8 readByte = rawData[o];

        if (readByte > 0x7F /*&& readByte <= 0xFF*/) {
            // Transparent pixels group
            for (int i = 0; i < (256 - readByte); i++) {
                // Add transparent pixel
                pixelLine.push_back(D1GfxPixel::transparentPixel());
            }
        } else /*if (readByte >= 0x00 && readByte <= 0x7F)*/ {
            // Palette indices group
            if ((o + readByte) >= rawData.size()) {
                pixelLine.push_back(D1GfxPixel::transparentPixel()); // ensure pixelLine is not empty to report error at the end
                break;
            }

            // if (readByte == 0x00) {
            //    dProgressWarn() << QString("Invalid CEL frame data (0x00 found)");
            // }
            for (int i = 0; i < readByte; i++) {
                // Go to the next palette index offset
                o++;
                // Add opaque pixel
                pixelLine.push_back(D1GfxPixel::colorPixel(rawData[o]));
            }
        }

        if (pixelLine.size() >= width) {
            if (pixelLine.size() > width) {
                break;
            }
            pixels.push_back(std::move(pixelLine));
            pixelLine.clear();
        }
    }
    if (!pixelLine.empty()) {
        return false;
    }

    for (auto it = pixels.rbegin(); it != pixels.rend(); ++it) {
        frame.pixels.push_back(std::move(*it));
    }
    frame.width = width;
    frame.height = frame.pixels.size();
    return true;
}
