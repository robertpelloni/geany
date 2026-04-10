#pragma once

#include <bobgui/bobgui.h>

/* First, add the boilerplate for the object itself.
 */
#define BOBGUI_TYPE_PUZZLE_PIECE (bobgui_puzzle_piece_get_type ())
G_DECLARE_FINAL_TYPE (BobguiPuzzlePiece, bobgui_puzzle_piece, BOBGUI, PUZZLE_PIECE, GObject)

/* Then, declare all constructors */
GdkPaintable *  bobgui_puzzle_piece_new            (GdkPaintable           *puzzle,
                                                 guint                   x,
                                                 guint                   y,
                                                 guint                   width,
                                                 guint                   height);

/* Next, add the getters and setters for object properties */
GdkPaintable *  bobgui_puzzle_piece_get_puzzle     (BobguiPuzzlePiece         *self);
guint           bobgui_puzzle_piece_get_x          (BobguiPuzzlePiece         *self);
guint           bobgui_puzzle_piece_get_y          (BobguiPuzzlePiece         *self);
