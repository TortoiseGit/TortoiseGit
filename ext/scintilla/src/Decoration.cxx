/** @file Decoration.cxx
 ** Visual elements added over text.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdarg>

#include <stdexcept>
#include <vector>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "Scintilla.h"
#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "Decoration.h"

using namespace Scintilla;

namespace {

class Decoration : public IDecoration {
	int indicator;
public:
	RunStyles<Sci::Position, int> rs;

	explicit Decoration(int indicator_) : indicator(indicator_) {
	}
	~Decoration() {
	}

	bool Empty() const override {
		return (rs.Runs() == 1) && (rs.AllSameAs(0));
	}
	int Indicator() const override {
		return indicator;
	}
	Sci::Position Length() const override {
		return rs.Length();
	}
	int ValueAt(Sci::Position position) const override {
		return rs.ValueAt(static_cast<Sci::Position>(position));
	}
	Sci::Position StartRun(Sci::Position position) const override {
		return rs.StartRun(static_cast<Sci::Position>(position));
	}
	Sci::Position EndRun(Sci::Position position) const override {
		return rs.EndRun(static_cast<Sci::Position>(position));
	}
	void SetValueAt(Sci::Position position, int value) override {
		rs.SetValueAt(static_cast<Sci::Position>(position), value);
	}
	void InsertSpace(Sci::Position position, Sci::Position insertLength) override {
		rs.InsertSpace(static_cast<Sci::Position>(position), static_cast<Sci::Position>(insertLength));
	}
	Sci::Position Runs() const override {
		return rs.Runs();
	}
};

class DecorationList : public IDecorationList {
	int currentIndicator;
	int currentValue;
	Decoration *current;	// Cached so FillRange doesn't have to search for each call.
	Sci::Position lengthDocument;
	// Ordered by indicator
	std::vector<std::unique_ptr<Decoration>> decorationList;
	std::vector<const IDecoration*> decorationView;	// Read-only view of decorationList
	bool clickNotified;

	Decoration *DecorationFromIndicator(int indicator);
	Decoration *Create(int indicator, Sci::Position length);
	void Delete(int indicator);
	void DeleteAnyEmpty();
	void SetView();
public:

	DecorationList();
	~DecorationList();

	const std::vector<const IDecoration*> &View() const override {
		return decorationView;
	}

	void SetCurrentIndicator(int indicator) override;
	int GetCurrentIndicator() const override { return currentIndicator; }

	void SetCurrentValue(int value) override;
	int GetCurrentValue() const override { return currentValue; }

	// Returns changed=true if some values may have changed
	FillResult<Sci::Position> FillRange(Sci::Position position, int value, Sci::Position fillLength) override;

	void InsertSpace(Sci::Position position, Sci::Position insertLength) override;
	void DeleteRange(Sci::Position position, Sci::Position deleteLength) override;

	void DeleteLexerDecorations() override;

	int AllOnFor(Sci::Position position) const override;
	int ValueAt(int indicator, Sci::Position position) override;
	Sci::Position Start(int indicator, Sci::Position position) override;
	Sci::Position End(int indicator, Sci::Position position) override;

	bool ClickNotified() const override {
		return clickNotified;
	}
	void SetClickNotified(bool notified) override {
		clickNotified = notified;
	}
};

DecorationList::DecorationList() : currentIndicator(0), currentValue(1), current(nullptr),
	lengthDocument(0), clickNotified(false) {
}

DecorationList::~DecorationList() {
	current = nullptr;
}

Decoration *DecorationList::DecorationFromIndicator(int indicator) {
	for (const std::unique_ptr<Decoration> &deco : decorationList) {
		if (deco->Indicator() == indicator) {
			return deco.get();
		}
	}
	return nullptr;
}

Decoration *DecorationList::Create(int indicator, Sci::Position length) {
	currentIndicator = indicator;
	std::unique_ptr<Decoration> decoNew = std::make_unique<Decoration>(indicator);
	decoNew->rs.InsertSpace(0, length);

	std::vector<std::unique_ptr<Decoration>>::iterator it = std::lower_bound(
		decorationList.begin(), decorationList.end(), decoNew,
		[](const std::unique_ptr<Decoration> &a, const std::unique_ptr<Decoration> &b) {
		return a->Indicator() < b->Indicator();
	});
	std::vector<std::unique_ptr<Decoration>>::iterator itAdded =
		decorationList.insert(it, std::move(decoNew));

	SetView();

	return itAdded->get();
}

void DecorationList::Delete(int indicator) {
	decorationList.erase(std::remove_if(decorationList.begin(), decorationList.end(),
		[=](const std::unique_ptr<Decoration> &deco) {
		return deco->Indicator() == indicator;
	}), decorationList.end());
	current = nullptr;
	SetView();
}

void DecorationList::SetCurrentIndicator(int indicator) {
	currentIndicator = indicator;
	current = DecorationFromIndicator(indicator);
	currentValue = 1;
}

void DecorationList::SetCurrentValue(int value) {
	currentValue = value ? value : 1;
}

FillResult<Sci::Position> DecorationList::FillRange(Sci::Position position, int value, Sci::Position fillLength) {
	if (!current) {
		current = DecorationFromIndicator(currentIndicator);
		if (!current) {
			current = Create(currentIndicator, lengthDocument);
		}
	}
	const FillResult<Sci::Position> fr = current->rs.FillRange(position, value, fillLength);
	if (current->Empty()) {
		Delete(currentIndicator);
	}
	return fr;
}

void DecorationList::InsertSpace(Sci::Position position, Sci::Position insertLength) {
	const bool atEnd = position == lengthDocument;
	lengthDocument += insertLength;
	for (const std::unique_ptr<Decoration> &deco : decorationList) {
		deco->rs.InsertSpace(position, insertLength);
		if (atEnd) {
			deco->rs.FillRange(position, 0, insertLength);
		}
	}
}

void DecorationList::DeleteRange(Sci::Position position, Sci::Position deleteLength) {
	lengthDocument -= deleteLength;
	for (const std::unique_ptr<Decoration> &deco : decorationList) {
		deco->rs.DeleteRange(position, deleteLength);
	}
	DeleteAnyEmpty();
	if (decorationList.size() != decorationView.size()) {
		// One or more empty decorations deleted so update view.
		current = nullptr;
		SetView();
	}
}

void DecorationList::DeleteLexerDecorations() {
	decorationList.erase(std::remove_if(decorationList.begin(), decorationList.end(),
		[=](const std::unique_ptr<Decoration> &deco) {
		return deco->Indicator() < INDIC_CONTAINER;
	}), decorationList.end());
	current = nullptr;
	SetView();
}

void DecorationList::DeleteAnyEmpty() {
	if (lengthDocument == 0) {
		decorationList.clear();
	} else {
		decorationList.erase(std::remove_if(decorationList.begin(), decorationList.end(),
			[=](const std::unique_ptr<Decoration> &deco) {
			return deco->Empty();
		}), decorationList.end());
	}
}

void DecorationList::SetView() {
	decorationView.clear();
	for (const std::unique_ptr<Decoration> &deco : decorationList) {
		decorationView.push_back(deco.get());
	}
}

int DecorationList::AllOnFor(Sci::Position position) const {
	int mask = 0;
	for (const std::unique_ptr<Decoration> &deco : decorationList) {
		if (deco->rs.ValueAt(position)) {
			if (deco->Indicator() < INDIC_IME) {
				mask |= 1 << deco->Indicator();
			}
		}
	}
	return mask;
}

int DecorationList::ValueAt(int indicator, Sci::Position position) {
	const Decoration *deco = DecorationFromIndicator(indicator);
	if (deco) {
		return deco->rs.ValueAt(position);
	}
	return 0;
}

Sci::Position DecorationList::Start(int indicator, Sci::Position position) {
	const Decoration *deco = DecorationFromIndicator(indicator);
	if (deco) {
		return deco->rs.StartRun(position);
	}
	return 0;
}

Sci::Position DecorationList::End(int indicator, Sci::Position position) {
	const Decoration *deco = DecorationFromIndicator(indicator);
	if (deco) {
		return deco->rs.EndRun(position);
	}
	return 0;
}

}

namespace Scintilla {

std::unique_ptr<IDecoration> DecorationCreate(int indicator) {
	return std::make_unique<Decoration>(indicator);
}

std::unique_ptr<IDecorationList> DecorationListCreate() {
	return std::make_unique<DecorationList>();
}

}

