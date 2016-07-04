/* quadtree.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 04 Jul 2016, 08:50:22 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2016  Trinity Annabelle Quirk
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 * This file contains the quadtree method definitions.
 *
 * +-----+-----+
 * |     |     |
 * |  0  |  1  |
 * |     |     |
 * +-----+-----+
 * |     |     |
 * |  2  |  3  |
 * |     |     |
 * +-----+-----+
 *
 * Things to do
 *
 */

#include <algorithm>

#include "quadtree.h"

int Quadtree::classify(ui::panel *p)
{
    glm::ivec2 ul, lr;
    int retval = 0;

    p->get_va(ui::element::position, ui::position::x,  &ul.x,
              ui::element::position, ui::position::y,  &ul.y,
              ui::element::size,     ui::size::width,  &lr.x,
              ui::element::size,     ui::size::height, &lr.y, 0);
    /* Width and height are not absolute screen coords */
    lr += ul;
    retval |= this->which_quad(ul);
    retval |= this->which_quad(ul.x, lr.y);
    retval |= this->which_quad(lr.x, ul.y);
    retval |= this->which_quad(lr);
    return retval;
}

Quadtree::Quadtree(Quadtree *p,
                   glm::ivec2& pt1, glm::ivec2& pt2,
                   int max_depth, int cur_depth)
    : center(), min(), max(), contents()
{
    this->min.x = std::min(pt1.x, pt2.x);
    this->min.y = std::min(pt1.y, pt2.y);
    this->max.x = std::max(pt1.x, pt2.x);
    this->max.y = std::max(pt1.y, pt2.y);
    this->center = this->min + ((this->max - this->min) / 2);

    this->parent = p;
    if (cur_depth < max_depth)
    {
        glm::ivec2 tmp_pt;

        this->quadrant[0] = new Quadtree(this,
                                         this->center, this->min,
                                         max_depth, ++cur_depth);
        tmp_pt.x = this->max.x;
        tmp_pt.y = this->min.y;
        this->quadrant[1] = new Quadtree(this,
                                         this->center, tmp_pt,
                                         max_depth, cur_depth);
        tmp_pt.x = this->min.x;
        tmp_pt.y = this->max.y;
        this->quadrant[2] = new Quadtree(this,
                                         this->center, tmp_pt,
                                         max_depth, cur_depth);
        this->quadrant[3] = new Quadtree(this,
                                         this->center, this->max,
                                         max_depth, cur_depth);
    }
    else
    {
        this->quadrant[0] = NULL;
        this->quadrant[1] = NULL;
        this->quadrant[2] = NULL;
        this->quadrant[3] = NULL;
    }
}

Quadtree::~Quadtree()
{
    if (this->quadrant[0] != NULL) delete this->quadrant[0];
    if (this->quadrant[1] != NULL) delete this->quadrant[1];
    if (this->quadrant[2] != NULL) delete this->quadrant[2];
    if (this->quadrant[3] != NULL) delete this->quadrant[3];
}

void Quadtree::insert(ui::panel *obj)
{
}

void Quadtree::remove(ui::panel *obj)
{
}

void Quadtree::clear(void)
{
}

ui::panel *Quadtree::search(const glm::ivec2& pt)
{
    return NULL;
}
