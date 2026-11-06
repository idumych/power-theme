/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Tidy with square corners on its tab and frame, for a compositor that rounds
 * windows itself. It's Tidy's own code, under names of its own because
 * app_server loads every decorator add-on at once.
 */


#define TIDY_SQUARE_CORNERS
#define TidyDecorAddOn TidySquareDecorAddOn
#define TidyDecorator TidySquareDecorator

#include "TidyDecorator.cpp"
