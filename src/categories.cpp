/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "categories.h"

Categories::Categories()
  : m_categoriesById(DefaultEITCategories())
{
  // Copy over
  CategoryByIdMap::const_iterator it;
  for (it =  m_categoriesById.begin(); it != m_categoriesById.end(); ++it)
  {
    m_categoriesByName.insert(CategoryByNameMap::value_type(it->second, it->first));
  }
}

std::string Categories::Category(int category) const
{
  CategoryByIdMap::const_iterator it = m_categoriesById.find(category);
  if (it != m_categoriesById.end())
    return it->second;
  return "";
}

int Categories::Category(const std::string &category) const
{
  CategoryByNameMap::const_iterator it = m_categoriesByName.find(category);
  if (it != m_categoriesByName.end())
    return it->second;
  return 0;
}

CategoryByIdMap Categories::DefaultEITCategories() const
{
  CategoryByIdMap categoryMap;
  categoryMap.insert(std::pair<int, std::string>(0x10, "Film"));
  categoryMap.insert(std::pair<int, std::string>(0x11, "Crime/Mystery"));
  categoryMap.insert(std::pair<int, std::string>(0x1F, "Drama"));
  categoryMap.insert(std::pair<int, std::string>(0x20, "News"));
  categoryMap.insert(std::pair<int, std::string>(0x23, "Documentary"));
  categoryMap.insert(std::pair<int, std::string>(0x40, "Sports"));
  categoryMap.insert(std::pair<int, std::string>(0x50, "Children"));
  categoryMap.insert(std::pair<int, std::string>(0x54, "Educational"));
  categoryMap.insert(std::pair<int, std::string>(0x60, "Music"));
  categoryMap.insert(std::pair<int, std::string>(0x70, "Arts/Culture"));
  categoryMap.insert(std::pair<int, std::string>(0x73, "Religion"));
  categoryMap.insert(std::pair<int, std::string>(0x90, "Science/Nature"));
  categoryMap.insert(std::pair<int, std::string>(0x55, "Animated"));
  categoryMap.insert(std::pair<int, std::string>(0xC8, "Adulty"));
  categoryMap.insert(std::pair<int, std::string>(0xC4, "Comedy"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Unknown"));

  return categoryMap;
}
