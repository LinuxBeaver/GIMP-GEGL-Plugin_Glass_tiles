/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Credit to Øyvind Kolås (pippin) for major GEGL contributions
 * 2024 Beaver, Glass Tiles

Test this plugin without installing by pasting this syntax into Gimp's GEGL graph filter.
This code will generate a static preview of the plugin

id=0 src aux=[ ref=0

grid  line-height=14 line-width=14
x=150 y=150 line-color=white
crop aux=[ ref=0 ]

gaussian-blur std-dev-x=3 std-dev-y=3 clip-extent=false abyss-policy=none

emboss elevation=0 azimuth=25 depth=0
opacity value=0.7
]
dst-over aux=[ ref=0 gaussian-blur std-dev-x=1 std-dev-y=1 ]
--end of syntax --

Fun fact, This filter is directly inspired by Pixelitor's and Photoshops Glass Tiles
also this filter is somewhat similar to my "plastic wrap" plugin that is also inspired by PS

 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES


property_int    (x, _("Tile width"), 200)
    description (_("Horizontal control of glass tile"))
    value_range (50, 500)
    ui_range    (150, 350)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "x")

property_int    (y, _("Tile height"), 200)
    description (_("Vertical control of glass tile"))
    value_range (50, 500)
    ui_range    (150, 350)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "y")

property_int    (x2, _("Tile horizontal offset"), 0)
    description (_("Horizontal offset of the tiles"))
    ui_range    (-64, 64)
    ui_meta     ("unit", "pixel-coordinate")
    ui_meta     ("axis", "x")

property_int    (y2, _("Tile vertical offset"), 0)
    description (_("Vertical offset of the tiles"))
    ui_range    (-64, 64)
    ui_meta     ("unit", "pixel-coordinate")
    ui_meta     ("axis", "y")


property_int    (lineradius, _("Tile radius"), 15)
    description (_("Radius of glass tile lines. Higher Values look inproper and 'grid like' on small images but go well on larger images"))
    value_range (13, 25)
    ui_range    (13, 25)
    ui_meta     ("unit", "pixel-distance")


property_double (azimuth, _("Tile azimuth"), 36.0)
   description (_("Emboss azimuth for for glass lines. "))
   value_range (20.0, 40.0)
   ui_range    (20.0, 40.0)
   ui_gamma    (3.0)
  ui_steps      (0.1, 0.50)


property_double (blurcontent, _("Blur content below the glass"), 0.0)
   description (_("Apply a gaussian blur filter on the subject/object that is behind the glass. At 0 this is entirely disabled"))
   value_range (0.0, 4.0)
   ui_range    (0.0, 3.0)
   ui_gamma    (3.0)
  ui_steps      (0.1, 0.500)

property_double (noise, _("Stained glass presence"), 0.0)
   description (_("The amount of stained glass noise is controlled here. At 0 there is no stained glass"))
   value_range (0.0, 1.0)
   ui_range    (0.0, 1.0)

property_seed    (noiseseed, _("Random seed of stained glass"), rand)
    description  (_("The random seed for the stained glass noise"))

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     glass_tiles
#define GEGL_OP_C_SOURCE glass-tiles.c

#include "gegl-op.h"

static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglColor *whitecolordefined = gegl_color_new ("#ffffff");

  GeglNode*input    = gegl_node_get_input_proxy (gegl, "input");

  GeglNode*output   = gegl_node_get_output_proxy (gegl, "output");

  GeglNode*idref = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  GeglNode*fix = gegl_node_new_child (gegl,
                                  "operation", "gegl:median-blur", "radius", 0, "abyss-policy", 0, 
                                  NULL);


  GeglNode*behind = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst-over",
                                  NULL);

 GeglNode*replace = gegl_node_new_child (gegl,
                                  "operation", "gegl:src",
                                  NULL);


  GeglNode*opacity = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity", "value", 0.9o,
                                  NULL);

  GeglNode*normal = gegl_node_new_child (gegl,
                                  "operation", "gimp:layer-mode", "layer-mode", 28,
                                  NULL);


  GeglNode*noise = gegl_node_new_child (gegl,
                                  "operation", "gegl:noise-cie-lch", "chroma-distance", 0.0, "holdness", 6,
                                  NULL);

  GeglNode*grid = gegl_node_new_child (gegl,
                                  "operation", "gegl:grid", "line-color", whitecolordefined,
                                  NULL);


  GeglNode*blur = gegl_node_new_child (gegl,
                                  "operation", "gegl:gaussian-blur",
                                         "clip-extent", FALSE,
                                         "abyss-policy", 0,
                                  NULL);

  GeglNode*lens = gegl_node_new_child (gegl,
                                  "operation", "gegl:lens-blur", "radius", 8.0,
                                  NULL);


  GeglNode*emboss = gegl_node_new_child (gegl,
                                  "operation", "gegl:emboss", "depth", 1, "elevation", 0.0, "azimuth", 25.0,
                                  NULL);

/*Replace blend mode contains everything except the behind blend mode*/
  gegl_node_link_many (input, replace, fix, behind, idref, normal,  output, NULL);
  gegl_node_connect (replace, "aux", opacity, "output");
/*Main content inside the replace blend mode*/
  gegl_node_link_many (grid, lens,  emboss, opacity, NULL);
/*The behind blend mode contains a copy of the original image that is blurred*/
  gegl_node_link_many (input, blur, NULL);
  gegl_node_connect (behind, "aux", blur, "output");
/*The normal blend mode contains noise that is on an opacity slider*/
  gegl_node_connect (normal, "aux", noise, "output");
  gegl_node_link_many (idref, noise, NULL);

 gegl_operation_meta_redirect (operation, "blurcontent", blur, "std-dev-x");
 gegl_operation_meta_redirect (operation, "blurcontent", blur, "std-dev-y");
 gegl_operation_meta_redirect (operation, "azimuth", emboss, "azimuth");
 gegl_operation_meta_redirect (operation, "noise", normal, "opacity");
 gegl_operation_meta_redirect (operation, "noiseseed", noise, "seed");
 gegl_operation_meta_redirect (operation, "lineradius", grid, "line-width");
 gegl_operation_meta_redirect (operation, "lineradius", grid, "line-height");
 gegl_operation_meta_redirect (operation, "x", grid, "x");
 gegl_operation_meta_redirect (operation, "y", grid, "y");
 gegl_operation_meta_redirect (operation, "x2", grid, "x-offset");
 gegl_operation_meta_redirect (operation, "y2", grid, "y-offset");

}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "lb:glass-tiles",
    "title",       _("Glass Tiled Window"),
    "reference-hash", "imthinkingmiku",
    "description", _("Make it look the image is behind a glass tile. Try using this with the blend mode lch lightness for interesting results"),
    "gimp:menu-path", "<Image>/Filters/Light and Shadow",
    "gimp:menu-label", _("Glass Tiled Window..."),
    NULL);
}

#endif
