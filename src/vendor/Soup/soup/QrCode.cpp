#include "QrCode.hpp"

#include <climits>

#include "BCanvas.hpp"
#include "Canvas.hpp"
#include "string.hpp"

NAMESPACE_SOUP
{
	using SegmentMode = QrCode::SegmentMode;
	using Segment = QrCode::Segment;
	using BitBuffer = QrCode::BitBuffer;

	/*---- Class QrSegment ----*/

	SegmentMode::SegmentMode(int mode, int cc0, int cc1, int cc2) :
		modeBits(mode) {
		numBitsCharCount[0] = cc0;
		numBitsCharCount[1] = cc1;
		numBitsCharCount[2] = cc2;
	}

	int SegmentMode::getModeBits() const {
		return modeBits;
	}

	int SegmentMode::numCharCountBits(int ver) const {
		return numBitsCharCount[(ver + 7) / 17];
	}

	const SegmentMode SegmentMode::NUMERIC(0x1, 10, 12, 14);
	const SegmentMode SegmentMode::ALPHANUMERIC(0x2, 9, 11, 13);
	const SegmentMode SegmentMode::BYTE(0x4, 8, 16, 16);
	const SegmentMode SegmentMode::KANJI(0x8, 8, 10, 12);
	const SegmentMode SegmentMode::ECI(0x7, 0, 0, 0);

	Segment Segment::makeBytes(const std::vector<uint8_t>& data)
	{
		SOUP_ASSERT(data.size() <= static_cast<unsigned int>(INT_MAX));

		BitBuffer bb;
		for (uint8_t b : data)
			bb.appendBits(b, 8);
		return Segment(SegmentMode::BYTE, static_cast<int>(data.size()), std::move(bb));
	}

	Segment Segment::makeNumeric(const char* digits) {
		BitBuffer bb;
		int accumData = 0;
		int accumCount = 0;
		int charCount = 0;
		for (; *digits != '\0'; digits++, charCount++) {
			char c = *digits;
			SOUP_ASSERT(c >= '0' && c <= '9');
			accumData = accumData * 10 + (c - '0');
			accumCount++;
			if (accumCount == 3) {
				bb.appendBits(static_cast<uint32_t>(accumData), 10);
				accumData = 0;
				accumCount = 0;
			}
		}
		if (accumCount > 0)  // 1 or 2 digits remaining
			bb.appendBits(static_cast<uint32_t>(accumData), accumCount * 3 + 1);
		return Segment(SegmentMode::NUMERIC, charCount, std::move(bb));
	}

	Segment Segment::makeAlphanumeric(const char* text) {
		BitBuffer bb;
		int accumData = 0;
		int accumCount = 0;
		int charCount = 0;
		for (; *text != '\0'; text++, charCount++) {
			const char* temp = std::strchr(ALPHANUMERIC_CHARSET, *text);
			SOUP_ASSERT(temp != nullptr); // String contains unencodable characters in alphanumeric mode
			accumData = accumData * 45 + static_cast<int>(temp - ALPHANUMERIC_CHARSET);
			accumCount++;
			if (accumCount == 2) {
				bb.appendBits(static_cast<uint32_t>(accumData), 11);
				accumData = 0;
				accumCount = 0;
			}
		}
		if (accumCount > 0)  // 1 character remaining
			bb.appendBits(static_cast<uint32_t>(accumData), 6);
		return Segment(SegmentMode::ALPHANUMERIC, charCount, std::move(bb));
	}

	std::vector<Segment> Segment::makeSegments(const char* text) {
		// Select the most efficient segment encoding automatically
		std::vector<Segment> result;
		if (*text != '\0')
		{
			if (isNumeric(text))
			{
				result.push_back(makeNumeric(text));
			}
			else if (isAlphanumeric(text))
			{
				result.push_back(makeAlphanumeric(text));
			}
			else
			{
				std::vector<uint8_t> bytes;
				for (; *text != '\0'; text++)
					bytes.push_back(static_cast<uint8_t>(*text));
				result.push_back(makeBytes(bytes));
			}
		}
		return result;
	}

	Segment Segment::makeEci(long assignVal) {
		BitBuffer bb;
		if (assignVal < 0) {
			SOUP_ASSERT_UNREACHABLE;
		}
		else if (assignVal < (1 << 7)) {
			bb.appendBits(static_cast<uint32_t>(assignVal), 8);
		}
		else if (assignVal < (1 << 14)) {
			bb.appendBits(2, 2);
			bb.appendBits(static_cast<uint32_t>(assignVal), 14);
		}
		else if (assignVal < 1000000L) {
			bb.appendBits(6, 3);
			bb.appendBits(static_cast<uint32_t>(assignVal), 21);
		}
		else {
			SOUP_ASSERT_UNREACHABLE;
		}
		return Segment(SegmentMode::ECI, 0, std::move(bb));
	}

	Segment::Segment(const SegmentMode& md, unsigned int numCh, const std::vector<bool>& dt)
		: mode(&md), numChars(numCh), data(dt)
	{
	}

	Segment::Segment(const SegmentMode& md, unsigned int numCh, std::vector<bool>&& dt)
		: mode(&md), numChars(numCh), data(std::move(dt))
	{
	}

	int Segment::getTotalBits(const std::vector<Segment>& segs, int version) {
		int result = 0;
		for (const Segment& seg : segs) {
			int ccbits = seg.mode->numCharCountBits(version);
			if (seg.numChars >= (1ul << ccbits))
				return -1;  // The segment's length doesn't fit the field's bit width
			if (4 + ccbits > INT_MAX - result)
				return -1;  // The sum will overflow an int type
			result += 4 + ccbits;
			if (seg.data.size() > static_cast<unsigned int>(INT_MAX - result))
				return -1;  // The sum will overflow an int type
			result += static_cast<int>(seg.data.size());
		}
		return result;
	}

	bool Segment::isNumeric(const char* text) {
		for (; *text != '\0'; ++text)
		{
			if (!string::isNumberChar(*text))
			{
				return false;
			}
		}
		return true;
	}

	bool Segment::isAlphanumeric(const char* text) {
		for (; *text != '\0'; text++) {
			if (std::strchr(ALPHANUMERIC_CHARSET, *text) == nullptr)
				return false;
		}
		return true;
	}

	const SegmentMode& Segment::getMode() const {
		return *mode;
	}

	int Segment::getNumChars() const {
		return numChars;
	}

	const std::vector<bool>& Segment::getData() const {
		return data;
	}

	const char* Segment::ALPHANUMERIC_CHARSET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";

	QrCode QrCode::encodeText(const char* text, ecc ecl)
	{
		std::vector<Segment> segs = Segment::makeSegments(text);
		return encodeSegments(segs, ecl);
	}

	QrCode QrCode::encodeText(const std::string& text, ecc ecl)
	{
		return encodeText(text.c_str());
	}

	QrCode QrCode::encodeBinary(const std::vector<uint8_t>& data, ecc ecl)
	{
		std::vector<Segment> segs{ Segment::makeBytes(data) };
		return encodeSegments(segs, ecl);
	}

	QrCode QrCode::encodeSegments(const std::vector<Segment>& segs, ecc ecl)
	{
		constexpr int minVersion = 1;
		constexpr int maxVersion = 40;
		constexpr int mask = -1;
		constexpr bool boostEcl = true;

		// Find the minimal version number to use
		int version, dataUsedBits;
		for (version = minVersion; ; version++) {
			int dataCapacityBits = getNumDataCodewords(version, ecl) * 8;  // Number of data bits available
			dataUsedBits = Segment::getTotalBits(segs, version);
			if (dataUsedBits != -1 && dataUsedBits <= dataCapacityBits)
				break;  // This version number is found to be suitable
			SOUP_ASSERT(version < maxVersion); // All versions in the range could not fit the given data
		}

		// Increase the error correction level while the data still fits in the current version number
		for (ecc newEcl : {ecc::MEDIUM, ecc::QUARTILE, ecc::HIGH}) {  // From low to high
			if (boostEcl && dataUsedBits <= getNumDataCodewords(version, newEcl) * 8)
				ecl = newEcl;
		}

		// Concatenate all segments to create the data bit string
		BitBuffer bb;
		for (const Segment& seg : segs) {
			bb.appendBits(static_cast<uint32_t>(seg.getMode().getModeBits()), 4);
			bb.appendBits(static_cast<uint32_t>(seg.getNumChars()), seg.getMode().numCharCountBits(version));
			bb.insert(bb.end(), seg.getData().begin(), seg.getData().end());
		}

		// Add terminator and pad up to a byte if applicable
		size_t dataCapacityBits = static_cast<size_t>(getNumDataCodewords(version, ecl)) * 8;
		bb.appendBits(0, std::min(4, static_cast<int>(dataCapacityBits - bb.size())));
		bb.appendBits(0, (8 - static_cast<int>(bb.size() % 8)) % 8);

		// Pad with alternating bytes until data capacity is reached
		for (uint8_t padByte = 0xEC; bb.size() < dataCapacityBits; padByte ^= 0xEC ^ 0x11)
			bb.appendBits(padByte, 8);

		// Pack bits into bytes in big endian
		std::vector<uint8_t> dataCodewords(bb.size() / 8);
		for (size_t i = 0; i < bb.size(); i++)
			dataCodewords.at(i >> 3) |= (bb.at(i) ? 1 : 0) << (7 - (i & 7));

		// Create the QR Code object
		return QrCode(version, ecl, dataCodewords, mask);
	}

	QrCode::QrCode(uint8_t ver, ecc ecl, const std::vector<uint8_t>& dataCodewords, int8_t msk)
		: version(ver), errorCorrectionLevel(ecl)
	{
		size = (ver * 4 + 17);
		size_t sz = static_cast<size_t>(size);
		modules = std::vector<bool>(sz * sz);  // Initially all light
		isFunction = std::vector<bool>(sz * sz);

		// Compute ECC, draw modules
		drawFunctionPatterns();
		const std::vector<uint8_t> allCodewords = addEccAndInterleave(dataCodewords);
		drawCodewords(allCodewords);

		// Do masking
		if (msk == -1) {  // Automatically choose best mask
			long minPenalty = LONG_MAX;
			for (int i = 0; i < 8; i++) {
				applyMask(i);
				drawFormatBits(i);
				long penalty = getPenaltyScore();
				if (penalty < minPenalty) {
					msk = i;
					minPenalty = penalty;
				}
				applyMask(i);  // Undoes the mask due to XOR
			}
		}
		mask = msk;
		applyMask(msk);  // Apply the final choice of mask
		drawFormatBits(msk);  // Overwrite old format bits

		isFunction.clear();
		isFunction.shrink_to_fit();
	}

	uint8_t QrCode::getVersion() const
	{
		return version;
	}

	uint8_t QrCode::getSize() const
	{
		return size;
	}

	QrCode::ecc QrCode::getErrorCorrectionLevel() const
	{
		return errorCorrectionLevel;
	}

	int8_t QrCode::getMask() const
	{
		return mask;
	}

	bool QrCode::getModule(int x, int y) const
	{
		return 0 <= x && x < size && 0 <= y && y < size && getModuleInbounds(x, y);
	}

	Canvas QrCode::toCanvas(unsigned int border, bool black_bg) const
	{
		if (black_bg)
		{
			return toCanvas(border, Rgb::WHITE, Rgb::BLACK);
		}
		return toCanvas(border, Rgb::BLACK, Rgb::WHITE);
	}

	Canvas QrCode::toCanvas(unsigned int border, Rgb fg, Rgb bg) const
	{
		Canvas c(getSize() + (border * 2));
		c.fill(bg);

		for (int qr_y = 0, canvas_y = border; qr_y != getSize(); ++qr_y, ++canvas_y)
		{
			for (int qr_x = 0, canvas_x = border; qr_x != getSize(); ++qr_x, ++canvas_x)
			{
				if (getModule(qr_x, qr_y))
				{
					c.set(canvas_x, canvas_y, fg);
				}
			}
		}

		return c;
	}

	Canvas QrCode::toCanvas(const Canvas& fg, Rgb bg) const
	{
		Canvas c(getSize() * fg.width, getSize() * fg.height);
		c.fill(bg);

		for (uint8_t x = 0; x != getSize(); ++x)
		{
			for (uint8_t y = 0; y != getSize(); ++y)
			{
				if (getModule(x, y))
				{
					c.addCanvas(x * fg.width, y * fg.height, fg);
				}
			}
		}

		return c;
	}

	BCanvas QrCode::toBCanvas() const
	{
		return BCanvas(getSize(), getSize(), modules);
	}

	int QrCode::getFormatBits(ecc ecl)
	{
		switch (ecl)
		{
		default:
		case ecc::LOW:
			return 1;

		case ecc::MEDIUM:
			return 0;

		case ecc::QUARTILE:
			return 3;

		case ecc::HIGH:
			return 2;
		}
	}

	void QrCode::drawFunctionPatterns() {
		// Draw horizontal and vertical timing patterns
		for (int i = 0; i < size; i++) {
			setFunctionModule(6, i, i % 2 == 0);
			setFunctionModule(i, 6, i % 2 == 0);
		}

		// Draw 3 finder patterns (all corners except bottom right; overwrites some timing modules)
		drawFinderPattern(3, 3);
		drawFinderPattern(size - 4, 3);
		drawFinderPattern(3, size - 4);

		// Draw numerous alignment patterns
		const std::vector<int> alignPatPos = getAlignmentPatternPositions();
		size_t numAlign = alignPatPos.size();
		for (size_t i = 0; i < numAlign; i++) {
			for (size_t j = 0; j < numAlign; j++) {
				// Don't draw on the three finder corners
				if (!((i == 0 && j == 0) || (i == 0 && j == numAlign - 1) || (i == numAlign - 1 && j == 0)))
					drawAlignmentPattern(alignPatPos.at(i), alignPatPos.at(j));
			}
		}

		// Draw configuration data
		drawFormatBits(0);  // Dummy mask value; overwritten later in the constructor
		drawVersion();
	}

	void QrCode::drawFormatBits(int msk) {
		// Calculate error correction code and pack bits
		int data = getFormatBits(errorCorrectionLevel) << 3 | msk;  // errCorrLvl is uint2, msk is uint3
		int rem = data;
		for (int i = 0; i < 10; i++)
			rem = (rem << 1) ^ ((rem >> 9) * 0x537);
		int bits = (data << 10 | rem) ^ 0x5412;  // uint15

		// Draw first copy
		for (int i = 0; i <= 5; i++)
			setFunctionModule(8, i, getBit(bits, i));
		setFunctionModule(8, 7, getBit(bits, 6));
		setFunctionModule(8, 8, getBit(bits, 7));
		setFunctionModule(7, 8, getBit(bits, 8));
		for (int i = 9; i < 15; i++)
			setFunctionModule(14 - i, 8, getBit(bits, i));

		// Draw second copy
		for (int i = 0; i < 8; i++)
			setFunctionModule(size - 1 - i, 8, getBit(bits, i));
		for (int i = 8; i < 15; i++)
			setFunctionModule(8, size - 15 + i, getBit(bits, i));
		setFunctionModule(8, size - 8, true);  // Always dark
	}

	void QrCode::drawVersion() {
		if (version < 7)
			return;

		// Calculate error correction code and pack bits
		int rem = version;  // version is uint6, in the range [7, 40]
		for (int i = 0; i < 12; i++)
			rem = (rem << 1) ^ ((rem >> 11) * 0x1F25);
		long bits = static_cast<long>(version) << 12 | rem;  // uint18

		// Draw two copies
		for (int i = 0; i < 18; i++) {
			bool bit = getBit(bits, i);
			int a = size - 11 + i % 3;
			int b = i / 3;
			setFunctionModule(a, b, bit);
			setFunctionModule(b, a, bit);
		}
	}

	void QrCode::drawFinderPattern(int x, int y) {
		for (int dy = -4; dy <= 4; dy++) {
			for (int dx = -4; dx <= 4; dx++) {
				int dist = std::max(std::abs(dx), std::abs(dy));  // Chebyshev/infinity norm
				int xx = x + dx, yy = y + dy;
				if (0 <= xx && xx < size && 0 <= yy && yy < size)
					setFunctionModule(xx, yy, dist != 2 && dist != 4);
			}
		}
	}

	void QrCode::drawAlignmentPattern(int x, int y) {
		for (int dy = -2; dy <= 2; dy++) {
			for (int dx = -2; dx <= 2; dx++)
				setFunctionModule(x + dx, y + dy, std::max(std::abs(dx), std::abs(dy)) != 1);
		}
	}

	void QrCode::setFunctionModule(int x, int y, bool isDark) {
		size_t ux = static_cast<size_t>(x);
		size_t uy = static_cast<size_t>(y);
		modules.at(uy * size + ux) = isDark;
		isFunction.at(uy * size + ux) = true;
	}

	bool QrCode::getModuleInbounds(int x, int y) const {
		return modules[static_cast<size_t>(y) * size + static_cast<size_t>(x)];
	}

	std::vector<uint8_t> QrCode::addEccAndInterleave(const std::vector<uint8_t>& data) const {
		SOUP_ASSERT(data.size() == static_cast<unsigned int>(getNumDataCodewords(version, errorCorrectionLevel)));

		// Calculate parameter numbers
		int numBlocks = NUM_ERROR_CORRECTION_BLOCKS[static_cast<int>(errorCorrectionLevel)][version];
		int blockEccLen = ECC_CODEWORDS_PER_BLOCK[static_cast<int>(errorCorrectionLevel)][version];
		int rawCodewords = getNumRawDataModules(version) / 8;
		int numShortBlocks = numBlocks - rawCodewords % numBlocks;
		int shortBlockLen = rawCodewords / numBlocks;

		// Split data into blocks and append ECC to each block
		std::vector<std::vector<uint8_t> > blocks;
		const std::vector<uint8_t> rsDiv = reedSolomonComputeDivisor(blockEccLen);
		for (int i = 0, k = 0; i < numBlocks; i++) {
			std::vector<uint8_t> dat(data.cbegin() + k, data.cbegin() + (k + shortBlockLen - blockEccLen + (i < numShortBlocks ? 0 : 1)));
			k += static_cast<int>(dat.size());
			const std::vector<uint8_t> ecc = reedSolomonComputeRemainder(dat, rsDiv);
			if (i < numShortBlocks)
				dat.push_back(0);
			dat.insert(dat.end(), ecc.cbegin(), ecc.cend());
			blocks.push_back(std::move(dat));
		}

		// Interleave (not concatenate) the bytes from every block into a single sequence
		std::vector<uint8_t> result;
		for (size_t i = 0; i < blocks.at(0).size(); i++) {
			for (size_t j = 0; j < blocks.size(); j++) {
				// Skip the padding byte in short blocks
				if (i != static_cast<unsigned int>(shortBlockLen - blockEccLen) || j >= static_cast<unsigned int>(numShortBlocks))
					result.push_back(blocks.at(j).at(i));
			}
		}
		return result;
	}

	void QrCode::drawCodewords(const std::vector<uint8_t>& data) {
		SOUP_ASSERT(data.size() == static_cast<unsigned int>(getNumRawDataModules(version) / 8));

		size_t i = 0;  // Bit index into the data
		// Do the funny zigzag scan
		for (int right = size - 1; right >= 1; right -= 2) {  // Index of right column in each column pair
			if (right == 6)
				right = 5;
			for (int vert = 0; vert < size; vert++) {  // Vertical counter
				for (int j = 0; j < 2; j++) {
					size_t x = static_cast<size_t>(right - j);  // Actual x coordinate
					bool upward = ((right + 1) & 2) == 0;
					size_t y = static_cast<size_t>(upward ? size - 1 - vert : vert);  // Actual y coordinate
					if (!isFunction.at(y * size + x) && i < data.size() * 8) {
						modules.at(y * size + x) = getBit(data.at(i >> 3), 7 - static_cast<int>(i & 7));
						i++;
					}
					// If this QR Code has any remainder bits (0 to 7), they were assigned as
					// 0/false/light by the constructor and are left unchanged by this method
				}
			}
		}
	}

	void QrCode::applyMask(int msk) {
		size_t sz = static_cast<size_t>(size);
		for (size_t y = 0; y < sz; y++) {
			for (size_t x = 0; x < sz; x++) {
				bool invert;
				switch (msk) {
				default:
				case 0:  invert = (x + y) % 2 == 0;                    break;
				case 1:  invert = y % 2 == 0;                          break;
				case 2:  invert = x % 3 == 0;                          break;
				case 3:  invert = (x + y) % 3 == 0;                    break;
				case 4:  invert = (x / 3 + y / 2) % 2 == 0;            break;
				case 5:  invert = x * y % 2 + x * y % 3 == 0;          break;
				case 6:  invert = (x * y % 2 + x * y % 3) % 2 == 0;    break;
				case 7:  invert = ((x + y) % 2 + x * y % 3) % 2 == 0;  break;
				}
				modules.at(y * sz + x) = modules.at(y * sz + x) ^ (invert & !isFunction.at(y * sz + x));
			}
		}
	}

	long QrCode::getPenaltyScore() const {
		long result = 0;

		// Adjacent modules in row having same color, and finder-like patterns
		for (int y = 0; y < size; y++) {
			bool runColor = false;
			int runX = 0;
			std::array<int, 7> runHistory = {};
			for (int x = 0; x < size; x++) {
				if (getModuleInbounds(x, y) == runColor) {
					runX++;
					if (runX == 5)
						result += PENALTY_N1;
					else if (runX > 5)
						result++;
				}
				else {
					finderPenaltyAddHistory(runX, runHistory);
					if (!runColor)
						result += finderPenaltyCountPatterns(runHistory) * PENALTY_N3;
					runColor = getModuleInbounds(x, y);
					runX = 1;
				}
			}
			result += finderPenaltyTerminateAndCount(runColor, runX, runHistory) * PENALTY_N3;
		}
		// Adjacent modules in column having same color, and finder-like patterns
		for (int x = 0; x < size; x++) {
			bool runColor = false;
			int runY = 0;
			std::array<int, 7> runHistory = {};
			for (int y = 0; y < size; y++) {
				if (getModuleInbounds(x, y) == runColor) {
					runY++;
					if (runY == 5)
						result += PENALTY_N1;
					else if (runY > 5)
						result++;
				}
				else {
					finderPenaltyAddHistory(runY, runHistory);
					if (!runColor)
						result += finderPenaltyCountPatterns(runHistory) * PENALTY_N3;
					runColor = getModuleInbounds(x, y);
					runY = 1;
				}
			}
			result += finderPenaltyTerminateAndCount(runColor, runY, runHistory) * PENALTY_N3;
		}

		// 2*2 blocks of modules having same color
		for (int y = 0; y < size - 1; y++) {
			for (int x = 0; x < size - 1; x++) {
				bool  color = getModuleInbounds(x, y);
				if (color == getModuleInbounds(x + 1, y) &&
					color == getModuleInbounds(x, y + 1) &&
					color == getModuleInbounds(x + 1, y + 1))
					result += PENALTY_N2;
			}
		}

		// Balance of dark and light modules
		int dark = 0;
		for (bool colour : modules) {
			if (colour) {
				dark++;
			}
		}
		int total = size * size;  // Note that size is odd, so dark/total != 1/2
		// Compute the smallest integer k >= 0 such that (45-5k)% <= dark/total <= (55+5k)%
		int k = static_cast<int>((std::abs(dark * 20L - total * 10L) + total - 1) / total) - 1;
		result += k * PENALTY_N4;
		return result;
	}

	std::vector<int> QrCode::getAlignmentPatternPositions() const {
		if (version == 1)
			return std::vector<int>();
		else {
			int numAlign = version / 7 + 2;
			int step = (version == 32) ? 26 :
				(version * 4 + numAlign * 2 + 1) / (numAlign * 2 - 2) * 2;
			std::vector<int> result;
			for (int i = 0, pos = size - 7; i < numAlign - 1; i++, pos -= step)
				result.insert(result.begin(), pos);
			result.insert(result.begin(), 6);
			return result;
		}
	}

	int QrCode::getNumRawDataModules(int ver) {
		int result = (16 * ver + 128) * ver + 64;
		if (ver >= 2) {
			int numAlign = ver / 7 + 2;
			result -= (25 * numAlign - 10) * numAlign - 55;
			if (ver >= 7)
				result -= 36;
		}
		return result;
	}

	int QrCode::getNumDataCodewords(int ver, ecc ecl) {
		return getNumRawDataModules(ver) / 8
			- ECC_CODEWORDS_PER_BLOCK[static_cast<int>(ecl)][ver]
			* NUM_ERROR_CORRECTION_BLOCKS[static_cast<int>(ecl)][ver];
	}

	std::vector<uint8_t> QrCode::reedSolomonComputeDivisor(int degree) {
		SOUP_ASSERT(degree >= 1 && degree <= 255);
		// Polynomial coefficients are stored from highest to lowest power, excluding the leading term which is always 1.
		// For example the polynomial x^3 + 255x^2 + 8x + 93 is stored as the uint8 array {255, 8, 93}.
		std::vector<uint8_t> result(static_cast<size_t>(degree));
		result.at(result.size() - 1) = 1;  // Start off with the monomial x^0

		// Compute the product polynomial (x - r^0) * (x - r^1) * (x - r^2) * ... * (x - r^{degree-1}),
		// and drop the highest monomial term which is always 1x^degree.
		// Note that r = 0x02, which is a generator element of this field GF(2^8/0x11D).
		uint8_t root = 1;
		for (int i = 0; i < degree; i++) {
			// Multiply the current product by (x - r^i)
			for (size_t j = 0; j < result.size(); j++) {
				result.at(j) = reedSolomonMultiply(result.at(j), root);
				if (j + 1 < result.size())
					result.at(j) ^= result.at(j + 1);
			}
			root = reedSolomonMultiply(root, 0x02);
		}
		return result;
	}

	std::vector<uint8_t> QrCode::reedSolomonComputeRemainder(const std::vector<uint8_t>& data, const std::vector<uint8_t>& divisor) {
		std::vector<uint8_t> result(divisor.size());
		for (uint8_t b : data) {  // Polynomial division
			uint8_t factor = b ^ result.at(0);
			result.erase(result.begin());
			result.push_back(0);
			for (size_t i = 0; i < result.size(); i++)
				result.at(i) ^= reedSolomonMultiply(divisor.at(i), factor);
		}
		return result;
	}

	uint8_t QrCode::reedSolomonMultiply(uint8_t x, uint8_t y) {
		// Russian peasant multiplication
		int z = 0;
		for (int i = 7; i >= 0; i--) {
			z = (z << 1) ^ ((z >> 7) * 0x11D);
			z ^= ((y >> i) & 1) * x;
		}
		return static_cast<uint8_t>(z);
	}

	int QrCode::finderPenaltyCountPatterns(const std::array<int, 7>& runHistory) const {
		int n = runHistory.at(1);
		bool core = n > 0 && runHistory.at(2) == n && runHistory.at(3) == n * 3 && runHistory.at(4) == n && runHistory.at(5) == n;
		return (core && runHistory.at(0) >= n * 4 && runHistory.at(6) >= n ? 1 : 0)
			+ (core && runHistory.at(6) >= n * 4 && runHistory.at(0) >= n ? 1 : 0);
	}

	int QrCode::finderPenaltyTerminateAndCount(bool currentRunColor, int currentRunLength, std::array<int, 7>& runHistory) const {
		if (currentRunColor) {  // Terminate dark run
			finderPenaltyAddHistory(currentRunLength, runHistory);
			currentRunLength = 0;
		}
		currentRunLength += size;  // Add light border to final run
		finderPenaltyAddHistory(currentRunLength, runHistory);
		return finderPenaltyCountPatterns(runHistory);
	}

	void QrCode::finderPenaltyAddHistory(int currentRunLength, std::array<int, 7>& runHistory) const {
		if (runHistory.at(0) == 0)
			currentRunLength += size;  // Add light border to initial run
		std::copy_backward(runHistory.cbegin(), runHistory.cend() - 1, runHistory.end());
		runHistory.at(0) = currentRunLength;
	}

	bool QrCode::getBit(long x, int i) {
		return ((x >> i) & 1) != 0;
	}

	/*---- Tables of constants ----*/

	const int QrCode::PENALTY_N1 = 3;
	const int QrCode::PENALTY_N2 = 3;
	const int QrCode::PENALTY_N3 = 40;
	const int QrCode::PENALTY_N4 = 10;

	const int8_t QrCode::ECC_CODEWORDS_PER_BLOCK[4][41] = {
		// Version: (note that index 0 is for padding, and is set to an illegal value)
		//0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40    Error correction level
		{-1,  7, 10, 15, 20, 26, 18, 20, 24, 30, 18, 20, 24, 26, 30, 22, 24, 28, 30, 28, 28, 28, 28, 30, 30, 26, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},  // Low
		{-1, 10, 16, 26, 18, 24, 16, 18, 22, 22, 26, 30, 22, 22, 24, 24, 28, 28, 26, 26, 26, 26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28},  // Medium
		{-1, 13, 22, 18, 26, 18, 24, 18, 22, 20, 24, 28, 26, 24, 20, 30, 24, 28, 28, 26, 30, 28, 30, 30, 30, 30, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},  // Quartile
		{-1, 17, 28, 22, 16, 22, 28, 26, 26, 24, 28, 24, 28, 22, 24, 24, 30, 28, 28, 26, 28, 30, 24, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},  // High
	};

	const int8_t QrCode::NUM_ERROR_CORRECTION_BLOCKS[4][41] = {
		// Version: (note that index 0 is for padding, and is set to an illegal value)
		//0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40    Error correction level
		{-1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4,  4,  4,  4,  4,  6,  6,  6,  6,  7,  8,  8,  9,  9, 10, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25},  // Low
		{-1, 1, 1, 1, 2, 2, 4, 4, 4, 5, 5,  5,  8,  9,  9, 10, 10, 11, 13, 14, 16, 17, 17, 18, 20, 21, 23, 25, 26, 28, 29, 31, 33, 35, 37, 38, 40, 43, 45, 47, 49},  // Medium
		{-1, 1, 1, 2, 2, 4, 4, 6, 6, 8, 8,  8, 10, 12, 16, 12, 17, 16, 18, 21, 20, 23, 23, 25, 27, 29, 34, 34, 35, 38, 40, 43, 45, 48, 51, 53, 56, 59, 62, 65, 68},  // Quartile
		{-1, 1, 1, 2, 4, 4, 4, 5, 6, 8, 8, 11, 11, 16, 16, 18, 16, 19, 21, 25, 25, 25, 34, 30, 32, 35, 37, 40, 42, 45, 48, 51, 54, 57, 60, 63, 66, 70, 74, 77, 81},  // High
	};

	BitBuffer::BitBuffer()
		: std::vector<bool>()
	{
	}

	void BitBuffer::appendBits(std::uint32_t val, int len)
	{
		/*if (len < 0 || len > 31 || val >> len != 0)
		{
			throw std::domain_error("Value out of range");
		}*/

		// Append bit by bit
		for (int i = len - 1; i >= 0; i--)
		{
			this->push_back(((val >> i) & 1) != 0);
		}
	}
}
