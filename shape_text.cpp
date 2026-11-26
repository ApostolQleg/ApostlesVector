#include "pch.h"
#include "shape_text.h"

static std::wstring EscapeXml(const std::wstring& value)
{
	std::wstring out;
	out.reserve(value.size());
	for (wchar_t ch : value)
	{
		switch (ch)
		{
		case L'&': out += L"&amp;";  break;
		case L'<': out += L"&lt;";   break;
		case L'>': out += L"&gt;";   break;
		case L'"': out += L"&quot;"; break;
		default:   out += ch;
		}
	}
	return out;
}

void Text::DrawGeometry(HDC hdc) const
{
	long l = 0, t = 0, r = 0, b = 0;
	NormalizeCoords(l, t, r, b);

	if (text.empty())
	{
		// Живий попередній перегляд рамки під час створення текстового блоку
		HPEN hPen = CreatePen(PS_DASH, 1, RGB(0, 120, 215));
		HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
		HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
		Rectangle(hdc, l, t, r, b);
		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);
		return;
	}

	const int savedDc = SaveDC(hdc);

	// Підтримка віддзеркалення тексту (якщо x1 > x2 або y1 > y2)
	const bool flipX = (x1 > x2);
	const bool flipY = (y1 > y2);
	if (flipX || flipY)
	{
		const double cx = (l + r) * 0.5;
		const double cy = (t + b) * 0.5;
		const FLOAT sx = flipX ? -1.0f : 1.0f;
		const FLOAT sy = flipY ? -1.0f : 1.0f;
		XFORM flipXf = {};
		flipXf.eM11 = sx;
		flipXf.eM22 = sy;
		flipXf.eDx = static_cast<FLOAT>(cx * (1.0 - sx));
		flipXf.eDy = static_cast<FLOAT>(cy * (1.0 - sy));
		ModifyWorldTransform(hdc, &flipXf, MWT_LEFTMULTIPLY);
	}

	const int fontHeight = 16 + style.borderWidth * 4;
	HFONT hFont = CreateFontW(
		fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

	HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));
	SetTextColor(hdc, style.hasBorder ? style.borderColor : RGB(0, 0, 0));
	SetBkMode(hdc, style.hasFill ? OPAQUE : TRANSPARENT);
	if (style.hasFill)
		SetBkColor(hdc, style.fillColor);

	RECT rc = { l, t, r, b };
	UINT fmt = DT_LEFT | DT_TOP | DT_NOCLIP;
	if (r - l >= 10) fmt |= DT_WORDBREAK;
	DrawTextW(hdc, text.c_str(), -1, &rc, fmt);

	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
	RestoreDC(hdc, savedDc);
}

void Text::Serialize(FILE* fout) const
{
	if (!fout) return;

	std::wstring escaped;
	escaped.reserve(text.size());
	for (wchar_t ch : text)
	{
		if (ch == L'\\') escaped += L"\\\\";
		else if (ch == L'\n') escaped += L"\\n";
		else if (ch != L'\r') escaped += ch;
	}

	fwprintf(fout, L"%s %ld %ld %ld %ld %lu %lu %d %d %d %.2f %s\n",
		GetTypeName(), x1, y1, x2, y2,
		static_cast<unsigned long>(style.borderColor),
		static_cast<unsigned long>(style.fillColor),
		style.hasBorder ? 1 : 0, style.hasFill ? 1 : 0,
		style.borderWidth, angleDeg, escaped.c_str());
}

bool Text::Deserialize(const WCHAR* params)
{
	if (!params) return false;
	unsigned long bCol = 0, fCol = 0;
	int borderFlag = 1, filled = 0, width = 1;
	double parsedAngle = 0.0;
	int offset = 0;

	if (swscanf_s(params, L"%ld %ld %ld %ld %lu %lu %d %d %d %lf%n",
		&x1, &y1, &x2, &y2, &bCol, &fCol, &borderFlag, &filled, &width, &parsedAngle, &offset) < 10)
	{
		return false;
	}

	style.borderColor = static_cast<COLORREF>(bCol);
	style.fillColor = static_cast<COLORREF>(fCol);
	style.hasBorder = (borderFlag != 0);
	style.hasFill = (filled != 0);
	style.borderWidth = (width > 0) ? width : 1;
	SetAngle(parsedAngle);

	const WCHAR* p = params + offset;
	if (*p == L' ') ++p;

	text.clear();
	while (*p && *p != L'\r' && *p != L'\n')
	{
		if (*p == L'\\')
		{
			++p;
			if (*p == L'\\') { text += L'\\'; ++p; }
			else if (*p == L'n') { text += L'\n'; ++p; }
			else if (*p) { text += *p; ++p; }
		}
		else
		{
			text += *p;
			++p;
		}
	}
	return true;
}

void Text::SerializeSvgBody(FILE* fout) const
{
	if (text.empty()) return;

	long l = 0, t = 0, r = 0, b = 0;
	NormalizeCoords(l, t, r, b);
	const int fontSize = 16 + style.borderWidth * 4;
	const bool flipX = (x1 > x2);
	const bool flipY = (y1 > y2);

	if (flipX || flipY)
	{
		const double cx = (l + r) * 0.5;
		const double cy = (t + b) * 0.5;
		fwprintf(fout, L"  <g transform=\"translate(%.1f %.1f) scale(%d %d) translate(%.1f %.1f)\">\n",
			cx, cy, flipX ? -1 : 1, flipY ? -1 : 1, -cx, -cy);
	}

	fwprintf(fout, L"  <text x=\"%ld\" y=\"%ld\" font-family=\"Segoe UI, sans-serif\" font-size=\"%d\" dominant-baseline=\"hanging\"",
		l, t, fontSize);
	WriteSvgColorAttr(fout, L"fill", style.hasBorder ? style.borderColor : RGB(0, 0, 0), true);
	fwprintf(fout, L">");

	size_t start = 0;
	bool first = true;
	while (true)
	{
		const size_t pos = text.find(L'\n', start);
		const std::wstring line = text.substr(start, (pos == std::wstring::npos) ? std::wstring::npos : pos - start);
		fwprintf(fout, L"<tspan x=\"%ld\" dy=\"%s\">%s</tspan>",
			l, first ? L"0" : L"1.25em", EscapeXml(line).c_str());
		first = false;
		if (pos == std::wstring::npos) break;
		start = pos + 1;
	}
	fwprintf(fout, L"</text>\n");

	if (flipX || flipY)
		fwprintf(fout, L"  </g>\n");
}