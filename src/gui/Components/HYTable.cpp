/*
 
 HyPhy - Hypothesis Testing Using Phylogenies.
 
 Copyright (C) 1997-now
 Core Developers:
 Sergei L Kosakovsky Pond (sergeilkp@icloud.com)
 Art FY Poon    (apoon@cfenet.ubc.ca)
 Steven Weaver (sweaver@temple.edu)
 
 Module Developers:
 Lance Hepler (nlhepler@gmail.com)
 Martin Smith (martin.audacis@gmail.com)
 
 Significant contributions from:
 Spencer V Muse (muse@stat.ncsu.edu)
 Simon DW Frost (sdf22@cam.ac.uk)
 
 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 */

#include "HYTableComponent.h"
#include "HYUtils.h"
#include "HYPlatformWindow.h"
#include "HYTableWindow.h"
#include "HYGraphicPane.h"
#include "HYEventTypes.h"
#include "HYDialogs.h"
#include "batchlan.h"

#include "math.h"

#ifdef    __HYPHYDMALLOC__
#include "dmalloc.h"
#endif

//extern     _HYColor   _BLACK_;


_HYColor tableDefaultBk  = {235,235,235},
         tableDefaultBk2 = {200,200,200};

_String  iconExportString ("[ICON]");

//__________________________________________________________________

_HYTable::_HYTable(_HYRect rel,Ptr w,long r, long c, long h, long v, long defType):
    _HYComponent (rel,w)
{
    SetTableSize (r,c,defType,h,v);
    SetTextColor ((_HYColor) {
        0,0,0
    });
    SetBackColor (tableDefaultBk);
    SetBackColor2(tableDefaultBk2);
    _HYFont      defFont;

#ifdef __MAC__
    defFont.face = "Times";
    defFont.size = 12;
#else
#ifdef __HYPHY_GTK__
    defFont.face = _HY_SANS_FONT;
    defFont.size = 11;
#else
    defFont.face = "Verdana";
    defFont.size = 12;
#endif
#endif

    defFont.style = HY_FONT_PLAIN;
    SetFont      (defFont);
    editCellID      = -1;
    selectionType   = 0;
    undoString      = nil;
    undoIndex       = undoIndex2 = -1;
    stretchWidth    = -1;
    stretchHeight   = -1;
}

//__________________________________________________________________

_HYTable::~_HYTable(void)
{
}

//__________________________________________________________________

void    _HYTable::SetTableSize(long r, long c, long defType, long h, long v)
{
    long      i;
    for (i=h; i<=c*h; i+=h) {
        horizontalSpaces<<i;
    }

    for (i=0; i<r; i++) {
        AddRow (-1,v,defType);
    }
}

//__________________________________________________________________

void    _HYTable::AddRow (long where, long h, long defType)
{
    long        i,w;
    if (where==-1) {
        where = verticalSpaces.lLength;
    }
    verticalSpaces<<0;
    for (i=verticalSpaces.lLength-1; i>where; i--) {
        verticalSpaces.lData[i]=verticalSpaces.lData[i-1]+h;
    }
    if (where) {
        verticalSpaces.lData[where]=verticalSpaces.lData[where-1]+h;
    } else {
        verticalSpaces.lData[0] = h;
    }

    w = where*horizontalSpaces.lLength;
    if ((defType&HY_TABLE_STATIC_TEXT)||(defType&HY_TABLE_EDIT_TEXT))
        // text
    {
        _String dummy;
        for (i=0; i<horizontalSpaces.lLength; i++) {
            cellData.InsertElement (&dummy,w,true);
            cellTypes.InsertElement((BaseRef)defType,w,false,false);
        }
    } else
        // icons
    {
        _SimpleList dummy;
        for (i=0; i<horizontalSpaces.lLength; i++) {
            cellData.InsertElement (&dummy,w,true);
            cellTypes.InsertElement((BaseRef)defType,w,false,false);
        }
    }

    stretchHeight = -1;
}

//__________________________________________________________________

void    _HYTable::RequestSpace (long r, long c)
{
    verticalSpaces.RequestSpace (r);
    cellData.RequestSpace (r*c);
    cellTypes.RequestSpace (r*c);
}

//__________________________________________________________________

void    _HYTable::DeleteRow (long where)
{
    long        i,h;
    if ((where<0)||(where>=verticalSpaces.lLength)) {
        return;
    }
    h = GetRowSpacing (where);

    for (i=where; i<verticalSpaces.lLength-1; i++) {
        verticalSpaces.lData[i]=verticalSpaces.lData[i+1]-h;
    }

    h = where*horizontalSpaces.lLength;
    for (i=0; i<horizontalSpaces.lLength; i++) {
        cellData.Delete (h);
        cellTypes.Delete(h);
    }
    verticalSpaces.Delete (verticalSpaces.lLength-1);

    stretchHeight = -1;
}

//__________________________________________________________________

void    _HYTable::AddColumn (long where, long h, long defType)
{
    long        i,w;
    if (where==-1) {
        where = horizontalSpaces.lLength;
    }
    horizontalSpaces<<0;
    for (i=horizontalSpaces.lLength-1; i>where; i--) {
        horizontalSpaces.lData[i]=horizontalSpaces.lData[i-1]+h;
    }
    if (where) {
        horizontalSpaces.lData[where]=horizontalSpaces.lData[where-1]+h;
    } else {
        horizontalSpaces.lData[0] = h;
    }
    w = where*horizontalSpaces.lLength;
    if ((defType&HY_TABLE_STATIC_TEXT)||(defType&HY_TABLE_EDIT_TEXT))
        // text
    {
        _String dummy;
        for (i=0; (i<horizontalSpaces.lLength)&&(w<cellData.lLength); i++, w+=verticalSpaces.lLength) {
            cellData.InsertElement (&dummy,w,true);
            cellTypes.InsertElement((BaseRef)defType,w,false,false);
        }
    } else
        // icons
    {
        _SimpleList dummy;
        for (i=0; (i<horizontalSpaces.lLength)&&(w<cellData.lLength); i++, w+=verticalSpaces.lLength) {
            cellData.InsertElement (&dummy,w,true);
            cellTypes.InsertElement((BaseRef)defType,w,false,false);
        }
    }

    stretchWidth = -1;
}

//__________________________________________________________________

void    _HYTable::DeleteColumn (long where)
{
    long        i,h;
    if ((where<0)||(where>=horizontalSpaces.lLength)) {
        return;
    }
    h = GetColumnSpacing (where);
    for (i=where; i<horizontalSpaces.lLength; i++) {
        horizontalSpaces.lData[i]=horizontalSpaces.lData[i+1]-h;
    }
    for (i=cellData.lLength-verticalSpaces.lLength-1+where; i>=0; i-=verticalSpaces.lLength) {
        cellData.Delete (i);
        cellTypes.Delete(i);
    }
    stretchWidth = -1;
}

//__________________________________________________________________

long    _HYTable::GetRowSpacing (long index)
{
    if (index>0) {
        return verticalSpaces.lData[index]-verticalSpaces.lData[index-1];
    }
    return verticalSpaces.lData[0];
}

//__________________________________________________________________

long    _HYTable::GetColumnSpacing (long index)
{
    if (index>0) {
        return horizontalSpaces.lData[index]-horizontalSpaces.lData[index-1];
    }
    return horizontalSpaces.lData[0];
}

//__________________________________________________________________

void    _HYTable::ClearTable (bool all)
{
    cellData.Clear();
    cellTypes.Clear();
    if (all) {
        horizontalSpaces.Clear();
    }
    verticalSpaces.Clear();
    hOrigin = vOrigin = 0;
}

//__________________________________________________________________

void    _HYTable::SetTableFromMx (_Matrix* data, long rowWidth, long colHeight, long cellType)
{
    ClearTable (true);
    long        rc = data->GetHDim(),
                cc = data->GetVDim(),
                spacer = rowWidth;

    verticalSpaces.RequestSpace (rc);

    for (long k=0; k<rc; k++) {
        verticalSpaces.lData[k] = spacer;
        spacer+=rowWidth;
    }
    verticalSpaces.lLength = rc;

    spacer = colHeight;
    horizontalSpaces.RequestSpace (cc);

    for (long k=0; k<cc; k++) {
        horizontalSpaces.lData[k] = spacer;
        spacer+=colHeight;
    }
    horizontalSpaces.lLength = cc;

    cellTypes.RequestSpace (cc*rc);
    for (long k=0; k<rc*cc; k++) {
        cellTypes.lData[k] = cellType;
    }

    cellTypes.lLength = rc*cc;

    cellData.RequestSpace (cc*rc);

    BaseRef * brf = (BaseRef*)(cellData.lData);

    for (long h=0; h<rc*cc; h++) {
        _String* cellData = new _String ((*data)(h/cc,h%cc));
        if (!(brf[h] = cellData)) {
            checkPointer (cellData);
        }
    }


    cellData.lLength = rc*cc;
}

//__________________________________________________________________

void    _HYTable::SetBackColor (_HYColor nc)
{
    backColor = nc;
    _SetBackColor (nc);
}

//__________________________________________________________________

void    _HYTable::SetBackColor2 (_HYColor nc)
{
    backColor2 = nc;
    _SetBackColor2 (nc);
}

//__________________________________________________________________

void    _HYTable::SetTextColor (_HYColor nc)
{
    textColor = nc;
}

//__________________________________________________________________

void    _HYTable::SetFont (_HYFont& nf)
{
    bool  t = !textFont.face.Equal(&nf.face);
    if ( textFont.size!=nf.size || textFont.style!=nf.style ||t) {
        textFont.size = nf.size;
        textFont.style = nf.style;
        if (t) {
            textFont.face = nf.face;
            _SetFont ();
        }
    }
}

//__________________________________________________________________

int     _HYTable::GetMaxW(void)
{
    long res = horizontalSpaces.lData[horizontalSpaces.lLength-1]+((settings.width&HY_COMPONENT_V_SCROLL)?HY_SCROLLER_WIDTH:0);
    if (res > 0x7ffff) {
        return 0x7ffff;
    }
    return res;
}

//__________________________________________________________________

int     _HYTable::GetMaxH(void)
{
    long res = verticalSpaces.lData[verticalSpaces.lLength-1]+((settings.width&HY_COMPONENT_H_SCROLL)?HY_SCROLLER_WIDTH:0);
    if (res > 0x7ffff) {
        return 0x7ffff;
    }
    return res;
}

//__________________________________________________________________

int     _HYTable::GetMaxLW(void)
{
    if (selectionType&HY_TABLE_DONT_GROW_HORIZ) {
        return _HYComponent::GetMaxW();
    } else if (selectionType&HY_TABLE_HORZ_STRETCH) {
        return 0x7ffff;
    } else {
        return GetMaxW();
    }
}

//__________________________________________________________________

int     _HYTable::GetMaxLH(void)
{
    if (selectionType&HY_TABLE_DONT_GROW_VERT) {
        return _HYComponent::GetMaxH();
    } else if (selectionType&HY_TABLE_VERT_STRETCH) {
        return 0x7ffff;
    } else {
        return GetMaxH();
    }
}

//__________________________________________________________________

bool        _HYTable::CheckForHSizeLocation (long h)
{
    h += hOrigin;
    for (long k = 0; k<horizontalSpaces.lLength; k++)
        if ((h>=horizontalSpaces.lData[k]-2)&&(h<=horizontalSpaces.lData[k]+2)) {
            return true;
        }
    return false;
}

//__________________________________________________________________

void        _HYTable::DragRow(long row, long after)
{
    if ((row>=0)&&(row<verticalSpaces.lLength)&&(after<verticalSpaces.lLength)&&(row!=after)) {
        cellTypes.Displace (row*horizontalSpaces.lLength,(row+1)*horizontalSpaces.lLength-1,
                            (after-row)*horizontalSpaces.lLength);
        cellData.Displace (row*horizontalSpaces.lLength,(row+1)*horizontalSpaces.lLength-1,
                           (after-row)*horizontalSpaces.lLength);
        _MarkForUpdate();
    }
}

//__________________________________________________________________

long        _HYTable::FindString (_String* s, long startat)
{
    for (long k=(startat>=0?startat:0); k<cellData.lLength; k++)
        if ((cellTypes.lData[k]&(HY_TABLE_STATIC_TEXT+HY_TABLE_EDIT_TEXT))&&
                (s->Equal((_String*)cellData(k))))

        {
            return k;
        }
    return -1;
}

//__________________________________________________________________

long        _HYTable::FindClickedTableCell (long hc,long vc,long& h, long& v)
{
    hc+=hOrigin;
    vc+=vOrigin;

    for (h = 0; h<horizontalSpaces.lLength; h++)
        if (hc<horizontalSpaces.lData[h]-2) {
            break;
        }
    if (h<horizontalSpaces.lLength) {
        for (v = 0; v<verticalSpaces.lLength; v++)
            if (vc<verticalSpaces.lData[v]-2) {
                break;
            }

        if (v<verticalSpaces.lLength) {
            long index = v*horizontalSpaces.lLength+h;
            if (cellTypes.lData[index]&HY_TABLE_PULLDOWN) {
                if ((hc>horizontalSpaces.lData[h]-15)&&(abs(2*(verticalSpaces.lData[v]-vc)-GetRowSpacing(v))<tPDMh)) {
                    return -2;
                }
            }
            return index;
        }
    }
    return -1;
}

//__________________________________________________________________

/*void      _HYTable::PlotAlignedIcon (_HYRect& store, long iconW, long iconH)
{

}*/

//__________________________________________________________________

void        _HYTable::EditBoxHandler (long index, _HYRect& r)
{
    if (_HasTextBox()) {
        _String editRes = _RetrieveTextValue();
        _KillTextBox();
        if (!editRes.Equal ((_String*)GetCellData(editCellID%horizontalSpaces.lLength,
                            editCellID/horizontalSpaces.lLength))) {
            SetCellData (&editRes,editCellID/horizontalSpaces.lLength,
                         editCellID%horizontalSpaces.lLength,
                         cellTypes.lData[editCellID],
                         true);
            if (messageRecipient) {
                messageRecipient->ProcessEvent (generateTableEditCellEvent(GetID(),editCellID));
            }
        }
        _SimpleList     dummy;
        dummy           <<  editCellID;
        _MarkCellsForUpdate (dummy);
        editCellID      = -1;
    }

    if (index>=0) {
        editCellID= index;
        long    v = index/horizontalSpaces.lLength;
        index     = index%horizontalSpaces.lLength;

        _HYRect textRect = r;

        if (index) {
            textRect.left += horizontalSpaces.lData[index-1] - hOrigin;
        } else {
            textRect.left -= hOrigin;
        }
        if (v) {
            textRect.top  += verticalSpaces.lData[v-1] - vOrigin;
        } else {
            textRect.top  -= vOrigin;
        }

        textRect.right     = r.left+horizontalSpaces.lData[index] - hOrigin;
        textRect.bottom    = r.top+verticalSpaces.lData[v] - vOrigin;

        if (settings.width&HY_COMPONENT_V_SCROLL) {
            if (textRect.right > r.right - HY_SCROLLER_WIDTH) {
                textRect.right = r.right - HY_SCROLLER_WIDTH;
            }
        } else if (textRect.right >= r.right) {
            textRect.right = r.right - 1;
        }

        if (settings.width&HY_COMPONENT_H_SCROLL) {
            if (textRect.bottom > r.bottom - HY_SCROLLER_WIDTH) {
                textRect.bottom = r.bottom - HY_SCROLLER_WIDTH;
            }
        } else if (textRect.bottom >= r.bottom) {
            textRect.bottom = r.bottom - 1;
        }


        textRect.left   +=2;
        //textRect.top  +=2;
        textRect.right  -=2;
        //textRect.bottom -=3;

        if ((textRect.right>textRect.left+textFont.size)&&(textRect.bottom>textRect.top+textFont.size)) {
            if (messageRecipient) {
                messageRecipient->ProcessEvent(generateKeyboardFocusEvent(GetID()));
            }
            _CreateTextBox (textRect, *(_String*)GetCellData(index,v));
            _SimpleList     dummy;
            dummy           <<  v*horizontalSpaces.lLength+index;
            _MarkCellsForUpdate (dummy);
        }
    } else {
        if ((messageRecipient)&&((selectionType&HY_TABLE_IS_FOCUSED)==0)) {
            messageRecipient->ProcessEvent(generateKeyboardFocusEvent(-1));
        }
    }
}

//__________________________________________________________________

void        _HYTable::ClearSelection (bool standAlone)
{
    _SimpleList modCells;
    for (long c = 0; c<cellTypes.lLength; c++) {
        if (cellTypes.lData[c]&HY_TABLE_SELECTED) {
            cellTypes.lData[c] -= HY_TABLE_SELECTED;
            modCells << c;
        }
    }
    if (modCells.lLength) {
        _MarkCellsForUpdate(modCells);
        if (_HasTextBox()) {
            _HYRect        dummy;
            EditBoxHandler (-1,dummy);
        }
        if (standAlone&&messageRecipient) {
            messageRecipient->ProcessEvent (generateTableChangeSelEvent(GetID()));
        }
    }
}

//__________________________________________________________________

void        _HYTable::SetRowSelection (const _SimpleList& rows)
{
    ClearSelection (false);
    _SimpleList modCells;
    modCells.RequestSpace (rows.lLength * horizontalSpaces.lLength);
    for (long t=0; t<rows.lLength; t++) {
        long rowN = rows.lData[t];
        if ((rowN>=0)&&(rowN<verticalSpaces.lLength)) {
            for (long k=rowN*horizontalSpaces.lLength; k<(rowN+1)*horizontalSpaces.lLength; k++) {
                if (!(cellTypes.lData[k]&HY_TABLE_CANTSELECT)) {
                    cellTypes.lData[k]|=HY_TABLE_SELECTED;
                    modCells << k;
                }
            }
        }
    }
    if (modCells.lLength) {
        for (long t=0; t<rows.lLength; t++) {
            _MarkRowForUpdate(rows.lData[t]);
        }

        if (_HasTextBox()) {
            _HYRect        dummy;
            EditBoxHandler (-1,dummy);
        }
        if (messageRecipient) {
            messageRecipient->ProcessEvent (generateTableChangeSelEvent(GetID()));
        }
    }
}

//__________________________________________________________________

void        _HYTable::SetColumnSelection (const _SimpleList& columns)
{
    ClearSelection (false);
    _SimpleList modCells;
    modCells.RequestSpace (columns.lLength * verticalSpaces.lLength);
    for (long t=0; t<columns.lLength; t++) {
        long rowN = columns.lData[t];
        if ((rowN>=0)&&(rowN<verticalSpaces.lLength)) {
            for (long k=rowN; k<cellTypes.lLength; k+=horizontalSpaces.lLength) {
                if (!(cellTypes.lData[k]&HY_TABLE_CANTSELECT)) {
                    cellTypes.lData[k]|=HY_TABLE_SELECTED;
                    modCells << k;
                }
            }
        }
    }
    if (modCells.lLength) {
        for (long t=0; t<columns.lLength; t++) {
            _MarkColumnForUpdate(columns.lData[t]);
        }

        if (_HasTextBox()) {
            _HYRect        dummy;
            EditBoxHandler (-1,dummy);
        }
        if (messageRecipient) {
            messageRecipient->ProcessEvent (generateTableChangeSelEvent(GetID()));
        }
    }
}


//__________________________________________________________________

void        _HYTable::ModifySelection (long h,long v, bool shift, bool control, bool message)
{
    long t = v*horizontalSpaces.lLength+h,t2;

    if (selectionType & HY_TABLE_SINGLE_SELECTION) {
        shift   = false;
        control = false;
    }

    if (cellTypes.lData[t]&HY_TABLE_CANTSELECT) {
        if (!shift) {
            ClearSelection();
            return;
        }
    }

    bool sel = cellTypes.lData[t]&HY_TABLE_SELECTED;

    _SimpleList  modCells;

    if (control) {
        _SimpleList  selection;
        GetSelection (selection);

        if (selection.lLength != 1) {
            return;
        }

        long    selRow = selection.lData[0]/horizontalSpaces.lLength,
                selCol = selection.lData[0]%horizontalSpaces.lLength,
                minRow,
                minCol,
                maxRow,
                maxCol;

        if (v<selRow) {
            minRow = v;
            maxRow = selRow;
        } else {
            maxRow = v;
            minRow = selRow;
        }

        if (h<selCol) {
            minCol = h;
            maxCol = selCol;
        } else {
            maxCol = h;
            minCol = selCol;
        }

        modCells.RequestSpace ((maxRow-minRow+1)*(maxCol-minCol+1));
        for (long r = minRow; r<=maxRow; r++)
            for (long c = r*horizontalSpaces.lLength+minCol; c<=r*horizontalSpaces.lLength+maxCol; c++) {
                if (!(cellTypes.lData[c]&HY_TABLE_CANTSELECT)) {
                    cellTypes.lData[c]|=HY_TABLE_SELECTED;
                }
                modCells<<c;
            }
    } else {
        if (shift) {
            if (!(cellTypes.lData[t]&HY_TABLE_CANTSELECT)) {
                if (sel) {
                    cellTypes.lData[t]-=HY_TABLE_SELECTED;
                } else {
                    cellTypes.lData[t]+=HY_TABLE_SELECTED;
                }
                modCells<<t;
            }
        } else {
            if (!sel) {
                for (long k=0; k<horizontalSpaces.lLength; k++)
                    for (long l=0; l<verticalSpaces.lLength; l++) {
                        t2 = l*horizontalSpaces.lLength+k;
                        if (t==t2) {
                            continue;
                        }
                        if (cellTypes.lData[t2]&HY_TABLE_CANTSELECT) {
                            continue;
                        }

                        if (cellTypes.lData[t2]&HY_TABLE_SELECTED) {
                            cellTypes.lData[t2]-=HY_TABLE_SELECTED;
                            modCells<<t2;
                        }
                    }
                if (!(cellTypes.lData[t]&HY_TABLE_CANTSELECT)) {
                    cellTypes.lData[t]+=HY_TABLE_SELECTED;
                }

                modCells<<t;
            } else {
                return;
            }
        }
    }

    if (selectionType&HY_TABLE_SEL_ROWS) {
        sel = cellTypes.lData[t]&HY_TABLE_SELECTED;
        if (sel)
            for (long k=v*horizontalSpaces.lLength; k<(v+1)*horizontalSpaces.lLength; k++) {
                if ((k==t)||(cellTypes.lData[k]&HY_TABLE_CANTSELECT)) {
                    continue;
                }
                cellTypes.lData[k]|=HY_TABLE_SELECTED;
                modCells<<k;
            }
        else
            for (long k=v*horizontalSpaces.lLength; k<(v+1)*horizontalSpaces.lLength; k++) {
                if ((k==t)||(cellTypes.lData[k]&HY_TABLE_CANTSELECT)) {
                    continue;
                }
                cellTypes.lData[k]&=HY_TABLE_DESELECT;
                modCells<<k;
            }
    } else if (selectionType&HY_TABLE_SEL_COLS) {
        sel = cellTypes.lData[t]&HY_TABLE_SELECTED;
        if (sel)
            for (long k=h; k<verticalSpaces.lLength*horizontalSpaces.lLength; k+=horizontalSpaces.lLength) {
                if ((k==t)||(cellTypes.lData[k]&HY_TABLE_CANTSELECT)) {
                    continue;
                }
                cellTypes.lData[k]|=HY_TABLE_SELECTED;
                modCells<<k;
            }
        else
            for (long k=h; k<verticalSpaces.lLength*horizontalSpaces.lLength; k+=horizontalSpaces.lLength) {
                if ((k==t)||(cellTypes.lData[k]&HY_TABLE_CANTSELECT)) {
                    continue;
                }
                cellTypes.lData[k]&=HY_TABLE_DESELECT;
                modCells<<k;
            }
    }

    if (modCells.lLength) {
        _MarkCellsForUpdate(modCells);
        if (_HasTextBox()) {
            _HYRect        dummy;
            EditBoxHandler (-1,dummy);
        }
        if (message&&messageRecipient) {
            messageRecipient->ProcessEvent (generateTableChangeSelEvent(GetID()));
        }
    }
}

//__________________________________________________________________

void        _HYTable::ExpungeSelection (void)
{
    _SimpleList  updateCells;
    for (long k=0; k<horizontalSpaces.lLength*verticalSpaces.lLength; k++) {
        if (cellTypes.lData[k]&HY_TABLE_SELECTED) {
            cellTypes.lData[k]&=HY_TABLE_DESELECT;
            updateCells<<k;
        }
    }
    _MarkCellsForUpdate (updateCells);
}

//__________________________________________________________________

void        _HYTable::SetColumnSpacing (long index, long h, bool update)
{
    if (h) {
        for (long k=index; k<horizontalSpaces.lLength; k++) {
            horizontalSpaces.lData[k]+=h;
        }
        if (update) {
            SetVisibleSize (rel);
            _MarkForUpdate();
        }
    }
    if (stretchWidth >= 0) {
        stretchWidth += h;
        if (stretchWidth < 0) {
            stretchWidth = -1;
        }
    }
}

//__________________________________________________________________

void        _HYTable::SetRowSpacing (long index, long v, bool update)
{
    if (v) {
        for (long k=index; k<verticalSpaces.lLength; k++) {
            verticalSpaces.lData[k]+=v;
        }
        if (update) {
            SetVisibleSize (rel);
            _MarkForUpdate();
        }

    }
    if (stretchHeight >= 0) {
        stretchHeight += v;
        if (stretchHeight < 0) {
            stretchHeight = -1;
        }
    }
}


//__________________________________________________________________

void        _HYTable::AutoFitColumn (long index, bool, bool increaseOnly)
{
    long maxWidth  = 0,
         cellWidth = 5;

    bool iconsOnly = true;

    for (long k=index; k<verticalSpaces.lLength*horizontalSpaces.lLength; k+=horizontalSpaces.lLength) {
        if (cellTypes.lData[k]&HY_TABLE_ICON) {
            cellWidth = ((_SimpleList*)cellData[k])->lData[1]+2;
        } else {
            _String*  cellValue = (_String*) cellData[k];
            if (cellTypes.lData[k]&HY_TABLE_BOLD) {
                textFont.style |= HY_FONT_BOLD;
            }
            if (cellTypes.lData[k]&HY_TABLE_ITALIC) {
                textFont.style |= HY_FONT_ITALIC;
            }
            cellWidth = GetVisibleStringWidth (*cellValue, textFont);
            if (cellTypes.lData[k]&HY_TABLE_PULLDOWN) {
                cellWidth += 5+tPDMw;
            }
            textFont.style = HY_FONT_PLAIN;
            iconsOnly = false;
        }
        if (cellWidth > maxWidth) {
            maxWidth = cellWidth;
        }
    }

    if (iconsOnly) {
        maxWidth+=2;
    } else {
        maxWidth+=textFont.size;
    }

    if (increaseOnly && (GetColumnSpacing(index)>maxWidth)) {
        return;
    }

    SetColumnSpacing (index,maxWidth-GetColumnSpacing(index),false);
}

//__________________________________________________________________

void        _HYTable::EnforceWidth (long width, long index, bool l)
{
    if ((index>=horizontalSpaces.lLength)||l) {
        index = horizontalSpaces.lLength-1;
    }

    if (horizontalSpaces.lData[index]<width) {
        long step = (width-horizontalSpaces.lData[index])/(index+1);
        if (!l)
            for (long k=0; k<index; k++) {
                horizontalSpaces.lData[k] += step*(k+1);
            }
        horizontalSpaces.lData[index] = width;
    }
}

//__________________________________________________________________

void        _HYTable::EnforceHeight (long height, long index, bool l)
{
    if ((index>=verticalSpaces.lLength)||l) {
        index = verticalSpaces.lLength-1;
    }

    if (verticalSpaces.lData[index]<height) {
        long step = (height-verticalSpaces.lData[index])/(index+1);
        if (!l)
            for (long k=0; k<index; k++) {
                verticalSpaces.lData[k] += step*(k+1);
            }
        verticalSpaces.lData[index] = height;
    }
}

//__________________________________________________________________

void        _HYTable::AutoFitWidth (void)
{
    for (long k=0; k<horizontalSpaces.lLength-1; k++) {
        AutoFitColumn (k,false);
    }
    AutoFitColumn (horizontalSpaces.lLength-1,true);
}


//__________________________________________________________________

void        _HYTable::AutoFitWidth (_HYTable& table2, long cshift)
{
    long k,
         w1,
         w2;

    AutoFitWidth();
    table2.AutoFitWidth();

    _SimpleList newWidths,
                oldWidths,
                oldWidths2;

    if (horizontalSpaces.lLength==table2.horizontalSpaces.lLength)
        for (k=0; k<horizontalSpaces.lLength; k++) {
            w1 = GetColumnSpacing (k);
            w2 = table2.GetColumnSpacing (k);
            if (k==horizontalSpaces.lLength-1) {
                  if ((settings.width&HY_COMPONENT_V_SCROLL)&&!(table2.settings.width&HY_COMPONENT_V_SCROLL)) {
                      w1 += HY_SCROLLER_WIDTH;
                  } else if (!(settings.width&HY_COMPONENT_V_SCROLL)&&(table2.settings.width&HY_COMPONENT_V_SCROLL)) {
                      w2 += HY_SCROLLER_WIDTH;
                  }
            }

            if (w1>w2) {
                newWidths << w1;
            } else {
                newWidths << w2;
            }
            oldWidths << w1;
            oldWidths2 << w2;
        }
    else
        for (k=0; k<horizontalSpaces.lLength; k++) {
            w1 = GetColumnSpacing (k);
            w2 = table2.GetColumnSpacing (k+cshift);

            if (w1>w2) {
                newWidths << w1;
            } else {
                newWidths << w2;
            }
            oldWidths << w1;
            oldWidths2 << w2;
        }

    long shift1 = 0,
         shift2 = 0;

    for (k=0; k<newWidths.lLength; k++) {
        shift1 += newWidths.lData[k] - oldWidths.lData[k];
        shift2 += newWidths.lData[k] - oldWidths2.lData[k];
        horizontalSpaces.lData[k] += shift1;
        table2.horizontalSpaces.lData[k+cshift] += shift2;
    }

    for (; k < table2.horizontalSpaces.lLength; k++) {
        table2.horizontalSpaces.lData[k] += shift2;
    }


}

//__________________________________________________________________
bool    _HYTable::ProcessEvent(_HYEvent* e)
{
    if (e->EventClass() == _hyScrollingEvent) {
        long h,v,k,w;
        _String firstArg = e->EventCode().Cut (0,(v=e->EventCode().Find(','))-1);
        h = firstArg.toNum();
        firstArg = e->EventCode().Cut (v+1,-1);
        v = firstArg.toNum();
        if (h||v) {
            if (h) {
                w = horizontalSpaces.lData[horizontalSpaces.lLength-1]-hSize;
                if (settings.width&HY_COMPONENT_V_SCROLL) {
                    w += HY_SCROLLER_WIDTH;
                }
                k = (double)h/MAX_CONTROL_VALUE * w;
                if (!k) {
                    k = h>0?5:-5;
                }
                _SetHScrollerPos (_GetHScrollerPos()+(double)k*MAX_CONTROL_VALUE/w-h);
                hOrigin += k;
                if (hOrigin<0) {
                    hOrigin -= k;
                    k=-hOrigin;
                    hOrigin = 0;
                    _SetHScrollerPos (0);
                } else if (hOrigin+hSize > w+hSize) {
                    hOrigin -= k;
                    k = w-hOrigin;
                    hOrigin = w;
                    _SetHScrollerPos (MAX_CONTROL_VALUE);
                }
                _HScrollTable(k);

                if (messageRecipient) {
                    messageRecipient->ProcessEvent (generateScrollEvent(h,0,GetID()));
                }
            } else {
                w = verticalSpaces.lData[verticalSpaces.lLength-1]-vSize;
                if (settings.width&HY_COMPONENT_H_SCROLL) {
                    w += HY_SCROLLER_WIDTH;
                }
                k = ((double)v/MAX_CONTROL_VALUE) * w;
                if (!k) {
                    k = v>0?10:-10;
                }

                _SetVScrollerPos (_GetVScrollerPos()+(double)k*MAX_CONTROL_VALUE/w-v);
                vOrigin += k;
                if (vOrigin<0) {
                    vOrigin -= k;
                    k = - vOrigin;
                    vOrigin = 0;
                    _SetVScrollerPos (0);
                } else if (vOrigin+vSize > w+vSize) {
                    vOrigin -= k;
                    k = w-vOrigin;
                    vOrigin = w;
                    _SetVScrollerPos (MAX_CONTROL_VALUE);
                }
                _VScrollTable(k);

                if (messageRecipient) {
                    messageRecipient->ProcessEvent (generateScrollEvent(0,v,GetID()));
                }
            }
        }
        DeleteObject(e);
        return true;
    }
    DeleteObject (e);
    return false;
}
//__________________________________________________________________

BaseRef _HYTable::GetCellData (long h, long v)
{
    return cellData(v*horizontalSpaces.lLength+h);
}

//__________________________________________________________________

void    _HYTable::SetCellData (BaseRef data, long h, long v, long type, bool copy)
{
    long idx = h*horizontalSpaces.lLength+v;
    cellTypes.lData[idx] = type;
    cellData.Replace (idx,data,copy);
}

//__________________________________________________________________

void    _HYTable::SetRowOrder (_SimpleList& order)
{
    if ((order.lLength == verticalSpaces.lLength)&&(order.lLength)) {
        _List           newData(cellData.lLength);
        _SimpleList     newTypes(cellTypes.lLength),
                        newSpaces;

        long            k,m,p;
        for (k=0; k<verticalSpaces.lLength; k++)
            for (m=0; m<horizontalSpaces.lLength; m++) {
                newTypes<<-1;
            }

        for (k=0; k<verticalSpaces.lLength; k++) {
            p = order.lData[k];
            for (m=0; m<horizontalSpaces.lLength; m++) {
                newTypes.lData[k*horizontalSpaces.lLength+m] = cellTypes.lData[p*horizontalSpaces.lLength+m];
                newData << GetCellData(m,p);
            }
        }
        if (newTypes.Find(-1)>=0) {
            return;
        }
        newSpaces << GetRowSpacing (order.lData[0]);
        for (k=1; k<verticalSpaces.lLength; k++) {
            newSpaces << GetRowSpacing (order.lData[k])+newSpaces.lData[k-1];
        }
        cellData.Clear();
        cellTypes.Clear();
        verticalSpaces.Clear();
        cellData.Duplicate(&newData);
        cellTypes.Duplicate(&newTypes);
        verticalSpaces.Duplicate(&newSpaces);

        _MarkForUpdate();
    }

}

//__________________________________________________________________

bool    _HYTable::ScrollToRow (long where)
{
    if ((where>=0)&&(where<verticalSpaces.lLength)) {
        long hs, hf, vs, vf;
        GetDisplayRange (&rel,vs,vf,hs,hf);
        if ((where<hs)||(where>=hf)) {
            if (where<hs) {
                if (where==0) {
                    vs = 0;
                } else {
                    vs = verticalSpaces.lData[where-1];
                }

                vs -= vOrigin;
            } else {
                vs = verticalSpaces.lData[where]-(rel.bottom-rel.top)/2-vOrigin;
            }

            vf = verticalSpaces.lData[verticalSpaces.lLength-1]-vSize;
            if (settings.width&HY_COMPONENT_H_SCROLL) {
                vf += HY_SCROLLER_WIDTH;
            }
            vs =(_Parameter) MAX_CONTROL_VALUE * (_Parameter)vs/(_Parameter)vf;
            _SetVScrollerPos (_GetVScrollerPos()+vs);
            ProcessEvent (generateScrollEvent(0,vs));
            return true;
        }
    }
    return false;
}

//__________________________________________________________________

bool    _HYTable::ScrollToColumn (long where)
{
    if ((where>=0)&&(where<horizontalSpaces.lLength)) {
        long hs, hf, vs, vf;
        GetDisplayRange (&rel,vs,vf,hs,hf);
        if ((where<vs)||(where>=vf)) {
            if (where<vs) {
                if (where==0) {
                    hs = 0;
                } else {
                    hs = verticalSpaces.lData[where-1];
                }

                hs -= hOrigin;
            } else {
                hs = horizontalSpaces.lData[where]-(rel.right-rel.left)/2-hOrigin;
            }

            hf = horizontalSpaces.lData[horizontalSpaces.lLength-1]-hSize;
            if (settings.width&HY_COMPONENT_V_SCROLL) {
                hf += HY_SCROLLER_WIDTH;
            }
            hs = (_Parameter)MAX_CONTROL_VALUE * (_Parameter)hs/(_Parameter)hf;
            _SetHScrollerPos (_GetHScrollerPos()+hs);
            ProcessEvent (generateScrollEvent(hs,0));
            return true;
        }
    }
    return false;
}

//__________________________________________________________________

void    _HYTable::SetVisibleSize (_HYRect r)
{
    EditBoxHandler  (-1,r);

    if (settings.width&HY_COMPONENT_V_SCROLL) {
        horizontalSpaces.lData[horizontalSpaces.lLength-1]+=HY_SCROLLER_WIDTH-1;
    }
    if (settings.width&HY_COMPONENT_H_SCROLL) {
        verticalSpaces.lData[verticalSpaces.lLength-1]+=HY_SCROLLER_WIDTH-1;
    }
    _HYComponent::SetVisibleSize(r);
    if (settings.width&HY_COMPONENT_V_SCROLL) {
        horizontalSpaces.lData[horizontalSpaces.lLength-1]-=HY_SCROLLER_WIDTH-1;
    }
    if (settings.width&HY_COMPONENT_H_SCROLL) {
        verticalSpaces.lData[verticalSpaces.lLength-1]-=HY_SCROLLER_WIDTH-1;
    }



    if (selectionType&HY_TABLE_HORZ_STRETCH) {
        if (stretchWidth>0) {
            SetColumnSpacing ( horizontalSpaces.lLength-1,-stretchWidth,false);
        }

        stretchWidth = horizontalSpaces.lData[horizontalSpaces.lLength-1];
        EnforceWidth (r.right-r.left+1-(settings.width&HY_COMPONENT_V_SCROLL?HY_SCROLLER_WIDTH:0), 0x7fffffff,true);
        stretchWidth = horizontalSpaces.lData[horizontalSpaces.lLength-1]-stretchWidth;
        //EnforceWidth (r.right-r.left+1, 0x7fffffff,true);
    }

    if (selectionType&HY_TABLE_VERT_STRETCH) {
        if (stretchHeight>0) {
            SetRowSpacing ( verticalSpaces.lLength-1,-stretchHeight,false);
        }

        stretchHeight = verticalSpaces.lData[verticalSpaces.lLength-1];
        EnforceHeight (r.bottom-r.top+1-(settings.width&HY_COMPONENT_H_SCROLL?HY_SCROLLER_WIDTH:0), 0x7fffffff,true);
        stretchHeight = verticalSpaces.lData[verticalSpaces.lLength-1]-stretchHeight;

    }

    long        t = GetMaxW();

    if (hOrigin+r.right-r.left>t) {
        hOrigin = t-r.right+r.left;
        _SetHScrollerPos (MAX_CONTROL_VALUE);
    }

    if (settings.width&HY_COMPONENT_H_SCROLL) {
        _Parameter c2 = hOrigin/(_Parameter)(t-r.right+r.left); // invisible section
        _SetHScrollerPos (c2*MAX_CONTROL_VALUE);
    }

    t = GetMaxH();
    if (vOrigin+r.bottom-r.top>t) {
        vOrigin = t-r.bottom+r.top;
        _SetVScrollerPos (MAX_CONTROL_VALUE);
    }

    if (settings.width&HY_COMPONENT_V_SCROLL) {
        _Parameter c2 = vOrigin/(_Parameter)(t-r.bottom+r.top); // invisible section
        _SetVScrollerPos (c2*MAX_CONTROL_VALUE);
    }
}

//__________________________________________________________________

void    _HYTable::GetDisplayRange (_HYRect* relRect, long& hs, long& hf, long& vs, long& vf )
{
    long    t = hOrigin;
    for (hs=0; hs<horizontalSpaces.lLength; hs++)
        if (horizontalSpaces.lData[hs]>t) {
            break;
        }
    if (hs==horizontalSpaces.lLength) {
        hs--;
    }
    t = relRect->right-relRect->left+hOrigin;
    if (settings.width&HY_COMPONENT_V_SCROLL) {
        t-=HY_SCROLLER_WIDTH;
    }

    for (hf=hs; hf<horizontalSpaces.lLength; hf++)
        if (horizontalSpaces.lData[hf]>t) {
            break;
        }
    if (hf==horizontalSpaces.lLength) {
        hf--;
    }

    t = vOrigin;
    for (vs=0; vs<verticalSpaces.lLength; vs++)
        if (verticalSpaces.lData[vs]>t) {
            break;
        }
    if (vs==verticalSpaces.lLength) {
        vs--;
    }
    t = relRect->bottom-relRect->top+vOrigin;
    if (settings.width&HY_COMPONENT_H_SCROLL) {
        t-=HY_SCROLLER_WIDTH;
    }

    for (vf=vs; vf<verticalSpaces.lLength; vf++)
        if (verticalSpaces.lData[vf]>t) {
            break;
        }
    if (vf==verticalSpaces.lLength) {
        vf--;
    }
}

//__________________________________________________________________

void    _HYTable::GetSelection (_SimpleList& rec)
{
    long selectedCount = 0;
    for (long k=0; k<cellTypes.lLength; k++)
        if (cellTypes.lData[k]&HY_TABLE_SELECTED) {
            selectedCount ++;
        }

    rec.RequestSpace (rec.lLength+selectedCount);
    for (long k=0; k<cellTypes.lLength; k++)
        if (cellTypes.lData[k]&HY_TABLE_SELECTED) {
            rec << k;
        }

}

//__________________________________________________________________

void    _HYTable::SetSelection (_SimpleList& rec, bool update)
{
    for (long k=0; k<rec.lLength; k++) {
        long idx = rec.lData[k];
        if (idx<cellTypes.lLength)
            if (!(cellTypes.lData[idx]&HY_TABLE_CANTSELECT)) {
                cellTypes.lData[idx] |= HY_TABLE_SELECTED;
            }
    }
    if (update&&messageRecipient) {
        messageRecipient->ProcessEvent (generateTableChangeSelEvent(GetID()));
    }
}

//__________________________________________________________________

void    _HYTable::InvertSelection (void)
{
    for (long idx=0; idx<cellTypes.lLength; idx++) {
        if (!(cellTypes.lData[idx]&HY_TABLE_CANTSELECT)) {
            if (cellTypes.lData[idx] & HY_TABLE_SELECTED) {
                cellTypes.lData[idx] -= HY_TABLE_SELECTED;
            } else {
                cellTypes.lData[idx] += HY_TABLE_SELECTED;
            }
        }
    }
    if (messageRecipient) {
        messageRecipient->ProcessEvent (generateTableChangeSelEvent(GetID()));
    }
}

//__________________________________________________________________

void    _HYTable::GetRowSelection (_SimpleList& rec, long shift)
{
    for (long k=0; k<cellTypes.lLength; k+=horizontalSpaces.lLength) {
        if (cellTypes.lData[k+shift]&HY_TABLE_SELECTED) {
            rec<<k/horizontalSpaces.lLength;
        }
    }
}

//__________________________________________________________________

void    _HYTable::ScanRowSelection (_SimpleList& rec)
{
    for (long k=0; k<cellTypes.lLength; k+=horizontalSpaces.lLength)
        for (long k2=0; k2<horizontalSpaces.lLength; k2++)
            if (cellTypes.lData[k+k2]&HY_TABLE_SELECTED) {
                rec << k/horizontalSpaces.lLength;
                break;
            }
}

//__________________________________________________________________

void    _HYTable::ScanColumnSelection (_SimpleList& rec)
{
    for (long k2=0; k2<horizontalSpaces.lLength; k2++)
        for (long k=k2; k<cellTypes.lLength; k+=horizontalSpaces.lLength)
            if (cellTypes.lData[k]&HY_TABLE_SELECTED) {
                rec << k2;
                break;
            }
}

//__________________________________________________________________

long    _HYTable::GetFirstRowSelection (void)
{
    _SimpleList rs;
    GetRowSelection (rs);
    if (rs.lLength) {
        return rs.lData[0];
    }
    return -1;
}

//__________________________________________________________________

bool    _HYTable::IsRowSelectionSimple (void)
{
  bool    res = false;
  for (long k=0; k<cellTypes.lLength; k+=horizontalSpaces.lLength) {
    if (cellTypes.lData[k]&HY_TABLE_SELECTED) {
      if (res) {
        return false;
      } else {
        res = true;
      }
    }
  }
  return res;
}

//__________________________________________________________________
bool    _HYTable::CanCopy (void)
{
    _SimpleList   sel;
    GetSelection (sel);

    return      sel.lLength;
}

//__________________________________________________________________
BaseRef _HYTable::CanPaste (_String& clip)
{
    _SimpleList   sel;
    GetSelection (sel);

    if (sel.lLength!=1) {
        return nil;
    }

    long rIdx = sel.lData[0]/horizontalSpaces.lLength,
         cIdx = sel.lData[0]%horizontalSpaces.lLength;

    _List        clipContents;

    _ElementaryCommand::ExtractConditions (clip,0,clipContents,';');

    if (verticalSpaces.lLength+1-rIdx < clipContents.lLength) {
        return nil;
    }

    _List*       clipData = new _List;

    if (!clipData) {
        return nil;
    }

    for (long k=0; k<clipContents.lLength; k++) {
        _List thisRow;
        _ElementaryCommand::ExtractConditions ((*(_String*)clipContents(k)),0,thisRow,',');
        if (horizontalSpaces.lLength+1-cIdx < thisRow.lLength) {
            DeleteObject (clipData);
            return       nil;
        }
        for (long kk=0; kk<thisRow.lLength; kk++) {
            ((_String*)thisRow(kk))->StripQuotes();
        }

        (*clipData) && & thisRow;
    }
    return clipData;
}

//__________________________________________________________________
_String*    _HYTable::HandleCopy (void)
{
    _String*     res = new _String (128L, true);

    _SimpleList  sel;
    GetSelection (sel);

    long         lastRow = -1;

    for (long k=0; k<sel.lLength; k++) {
        long rIndex = sel.lData[k]/horizontalSpaces.lLength,
             cIndex = sel.lData[k]%horizontalSpaces.lLength;

        if  (rIndex!=lastRow) {
            lastRow = rIndex;
            if (k) {
                (*res) << ';';
            }
        } else {
            (*res) << ',';
        }

        (*res) << '"';
        (*res) << (_String*)GetCellData (cIndex, rIndex);
        (*res) << '"';
    }

    res->Finalize();
    return       res;
}

//__________________________________________________________________

void    _HYTable::HandlePaste (BaseRef data)
{
    _List * pasteData = (_List*)data;

    _SimpleList  sel;
    GetSelection (sel);

    if (sel.lLength!=1) {
        return;
    }

    long rIdx = sel.lData[0]/horizontalSpaces.lLength,
         cIdx = sel.lData[0]%horizontalSpaces.lLength;

    _SimpleList taint;

    for (long k=0; k<pasteData->lLength; k++) {
        _List *thisRow = (_List*)(*pasteData)(k);

        for (long kk=0; kk<thisRow->lLength; kk++) {
            long idx = (rIdx+k)*horizontalSpaces.lLength + cIdx+kk;
            SetCellData ((_String*)(*thisRow)(kk),rIdx+k, cIdx+kk, cellTypes.lData[idx],true);
            taint << idx;
        }

    }

    _MarkCellsForUpdate (taint);

    if (messageRecipient) {
        messageRecipient->ProcessEvent (generateTableEditCellEvent(GetID(),sel.lData[0]));
    }

}


//__________________________________________________________________
void    _HYTable::GetColumnSelection (_SimpleList& rec)
{
    for (long k=0; k<horizontalSpaces.lLength; k++) {
        if (cellTypes.lData[k]&HY_TABLE_SELECTED) {
            rec<<k;
        }
    }
}

//__________________________________________________________________
void    _HYTable::UnfocusComponent (void)
{
    if (_HasTextBox()) {
        _HYRect        dummy;
        EditBoxHandler (-1,dummy);
    }
    if (selectionType&HY_TABLE_IS_FOCUSED) {
        selectionType -= HY_TABLE_IS_FOCUSED;
    }

    _HYComponent::UnfocusComponent();
}

//__________________________________________________________________
void    _HYTable::FocusComponent (void)
{
    if (!(selectionType&HY_TABLE_IS_FOCUSED)) {
        selectionType += HY_TABLE_IS_FOCUSED;
    }

    _SimpleList    cs;
    GetSelection   (cs);

    if (cs.lLength == 0) {
        long k;
        for (k=0; k<cellTypes.lLength; k++)
            if (cellTypes.lData[k]&HY_TABLE_CANTSELECT) {
                break;
            }

        if  (k<cellTypes.lLength) {
            cs << k;
            if ((selectionType&HY_TABLE_SEL_ROWS)==0) {
                cs.lData[0] /= horizontalSpaces.lLength;
                SetRowSelection (cs);
            }
            if (selectionType&HY_TABLE_SEL_COLS) {
                cs.lData[0] %= horizontalSpaces.lLength;
                SetColumnSelection (cs);
            } else {
                SetSelection (cs);
            }
        }
    }

    _FocusComponent ();
    _HYComponent::FocusComponent();
}

//__________________________________________________________________
void    _HYTable::_PrintTable (_HYTable* ch)
{
    _SimpleList rows,
                columns;

    long        k;

    for (k=0; k<horizontalSpaces.lLength; k++) {
        columns << k;
    }

    for (k=0; k<verticalSpaces.lLength; k++) {
        rows << k;
    }

    _PrintTable (columns, rows, ch);
}

//__________________________________________________________________
void    _HYTable::_PrintTable (_SimpleList& columns, _HYTable* ch)
{
    _SimpleList rows;

    long        k;

    for (k=0; k<verticalSpaces.lLength; k++) {
        rows << k;
    }

    _PrintTable (columns, rows, ch);
}

//__________________________________________________________________

void    _HYTable::SaveTable (_HYTable* ch, _HYTable *rh, long format, FILE * dest, _String& title)
{
    _SimpleList rows,
                columns;

    long        k;

    for (k=0; k<horizontalSpaces.lLength; k++) {
        columns << k;
    }

    for (k=0; k<verticalSpaces.lLength; k++) {
        rows << k;
    }

    SaveTable (ch,rh, format,dest,title, columns, rows);
}

//__________________________________________________________________

void    _HYTable::SaveTable (_HYTable* ch, _HYTable *rh, long format, FILE * dest, _String& title, _SimpleList& columns, _SimpleList& rows)
{
    long        i,j,k,t;
    if (columns.lLength&&rows.lLength&&dest) {
        switch (format) {
        case 0: // comma separated
        case 1: { // tab     separated
            char sep = format?'\t':',';
            if (ch) {
                if (rh) {
                    fputc (sep,dest);
                }
                for (i=0; i<columns.lLength-1; i++) {
                    ch->ExportCell (dest,format,0,columns.lData[i]);
                    fputc (sep,dest);
                }
                ch->ExportCell (dest,format,0,columns.lData[i]);
                fprintf (dest,"\n");
            }
            for (j=0; j<rows.lLength; j++) {
                if (rh) {
                    rh->ExportCell (dest,format,rows.lData[j],0);
                    fputc (sep,dest);
                }
                for (i=0; i<columns.lLength-1; i++) {
                    ExportCell (dest,format,rows.lData[j],columns.lData[i]);
                    fputc (sep,dest);
                }
                ExportCell (dest,format,rows.lData[j],columns.lData[i]);
                fprintf (dest,"\n");
            }
        }
        break;

        case 2: { // LaTEX table
            fprintf (dest, "{\\small\n\\begin{table}[t]\n\\begin{tabular}{");
            if (rh) {
                fprintf (dest,"r");
            }

            for (i=0; i<columns.lLength; i++) {
                fprintf (dest,"r");
            }

            fprintf (dest, "}\n\\hline\n");
            if (ch) {
                if (rh) {
                    fputc ('&',dest);
                }

                for (i=0; i<columns.lLength-1; i++) {
                    ch->ExportCell (dest,format,0,columns.lData[i]);
                    fputc ('&',dest);
                }
                ch->ExportCell (dest,format,0,columns.lData[i]);
                fprintf (dest, "\\\\\n\\hline\n");
            }
            for (j=0; j<rows.lLength; j++) {
                if (rh) {
                    rh->ExportCell (dest,format,rows.lData[j],0);
                    fputc ('&',dest);
                }
                for (i=0; i<columns.lLength-1; i++) {
                    ExportCell (dest,format,rows.lData[j],columns.lData[i]);
                    fputc ('&',dest);
                }
                ExportCell (dest,format,rows.lData[j],columns.lData[i]);
                fprintf (dest, "\\\\\n");
            }
            fprintf (dest,"\n\\end{tabular}\n\\caption{%s}\n\\end{table}\n}\n",title.sData);
        }
        break;

        case 3: { // HTML  table
            fprintf (dest, "<html>\n<head>\n<title>\n%s</title></head><body bgcolor = \"#FFFFFF\">\n<table border = \"0\" cellpadding = \"0\" cellspacing = \"1\">",title.sData);
            if (ch) {
                fprintf (dest, "\n<tr>\n");
                if (rh) {
                    fprintf (dest,"<td></td>");
                }
                for (i=0; i<columns.lLength; i++) {
                    ch->ExportCell (dest,format,0,columns.lData[i]);
                }
                fprintf (dest, "\n</tr>\n");
            }
            for (j=0; j<rows.lLength; j++) {
                fprintf (dest, "\n<tr>\n");
                if (rh) {
                    rh->ExportCell (dest,format,rows.lData[j],0);
                }
                for (i=0; i<columns.lLength; i++) {
                    ExportCell (dest,format,rows.lData[j],columns.lData[i]);
                }
                fprintf (dest, "\n</tr>\n");
            }
            fprintf (dest, "\n</table>\n</body>\n</html>\n");
        }
        break;

        case 4: { // ASCII art table
            _SimpleList width;

            for (i=0; i<columns.lLength; i++) {
                k = ch?ch->GetCellWidth (0,columns.lData[i]):2;

                for (j=0; j<rows.lLength; j++) {
                    t = GetCellWidth (rows.lData[j],columns.lData[i]);
                    if (t>k) {
                        k = t;
                    }
                }
                width << k;
            }

            fprintf (dest,"%s\n\n", title.sData);

            _String rowSeparator (128L, true);
            rowSeparator << '+';
            for (i=0; i<columns.lLength; i++) {
                for (j=-1; j<=width.lData[i]; j++) {
                    rowSeparator << '-';
                }
                rowSeparator << '+';
            }
            rowSeparator.Finalize();

            fprintf (dest, "%s\n", rowSeparator.sData);

            if (ch) {
                fputc ('|', dest);
                for (i=0; i<columns.lLength; i++) {
                    k = ch->GetCellWidth (0,columns.lData[i]);
                    ch->ExportCell (dest,format,0,columns.lData[i]);
                    for (t = k; t < width.lData[i]; t++) {
                        fputc (' ', dest);
                    }
                    fputc ('|', dest);
                }
                fprintf (dest, "\n%s\n", rowSeparator.sData);
            }

            for (j=0; j<rows.lLength; j++) {
                fputc ('|', dest);
                for (i=0; i<columns.lLength; i++) {
                    k = GetCellWidth (rows.lData[j],columns.lData[i]);
                    ExportCell (dest,format,rows.lData[j],columns.lData[i]);
                    for (t = k; t < width.lData[i]; t++) {
                        fputc (' ', dest);
                    }
                    fputc ('|', dest);
                }
                fprintf (dest, "\n%s\n", rowSeparator.sData);
            }
        }
        case 5: { // hyphy matrix
            fprintf (dest,"\n{\n");
            for (j=0; j<rows.lLength; j++) {
                fputc ('{',dest);
                for (i=0; i<columns.lLength-1; i++) {
                    ExportCell (dest,format,rows.lData[j],columns.lData[i]);
                    fputc (',',dest);
                }
                ExportCell (dest,format,rows.lData[j],columns.lData[i]);
                fprintf (dest,"}\n");
            }
            fprintf (dest,"}");
        }
        break;
        break;
        default: {
            _String wrongFormat ("SaveTable was passed an invalid format code");
            ProblemReport (wrongFormat);
        }
        }
    }
}

//__________________________________________________________________

long    _HYTable::GetCellWidth (long r, long c)
{
    long idx  = r*horizontalSpaces.lLength+c,
         type = cellTypes.lData[idx];

    if (type&HY_TABLE_ICON) {
        return iconExportString.sLength;
    }

    return ((_String*)GetCellData (c,r))->sLength;
}

//__________________________________________________________________

void    _HYTable::ExportCell (FILE* dest, long format, long r, long c)
{
    long idx  = r*horizontalSpaces.lLength+c,
         type = cellTypes.lData[idx];

    _String * contents;

    bool boldOn   = type&HY_TABLE_BOLD,
         italicOn = type&HY_TABLE_ITALIC;

    if (type&HY_TABLE_ICON) {
        contents = &iconExportString;
    } else {
        contents = ((_String*)GetCellData (c,r));
    }

    switch  (format) {
    case 2: { // LaTEX table
        if (boldOn) {
            fprintf (dest, "{\\bf ");
        }
        if (italicOn) {
            fprintf (dest, "{\\it ");
        }
    }
    break;
    case 3: { // HTML table
        fprintf (dest, "\n\t<td align = \"left\" valign = \"center\" bgcolor = \"#%s\">",
                 ((type&HY_TABLE_BEVELED)?backColor2:backColor).HTMLColor().getStr());
        if (boldOn) {
            fprintf (dest, "<b>");
        }
        if (italicOn) {
            fprintf (dest, "<i>");
        }
        fprintf (dest,"<font color = \"#%s\">&nbsp;",textColor.HTMLColor().getStr());
    }
    break;
    case 4:
        fputc (' ', dest);
        break;
    }

    if (format == 2) {
        fprintf (dest, "%s", contents->Replace ("_","\\_",true).sData);
    } else {
        fprintf (dest, "%s", contents->sData);
    }

    switch  (format) {
    case 2: { // LaTEX table
        if (italicOn) {
            fprintf (dest, "}");
        }
        if (boldOn) {
            fprintf (dest, "}");
        }
    }
    break;
    case 3: { // HTML table
        fprintf (dest,"</font>");
        if (italicOn) {
            fprintf (dest, "</i>");
        }
        if (boldOn) {
            fprintf (dest, "</b>");
        }
        fprintf (dest, "&nbsp;</td>");
    }
    case 4:
        fputc (' ', dest);
        break;
    }
}

//__________________________________________________________________

void    _HYTable::GetTableFormats (_List& rec)
{
    _String format ("Comma Separated");
    rec && & format;
    format = "Tab Separated";
    rec && & format;
    format = "LaTEX Table";
    rec && & format;
    format = "HTML Table";
    rec && & format;
    format = "ASCII Table";
    rec && & format;
    format = "HyPhy Matrix";
    rec && & format;
}

//__________________________________________________________________

void    _HYTable::HandleKeyMove (char dir, bool )
{
    bool         rowSel = selectionType&HY_TABLE_SEL_ROWS;

    _SimpleList  ts;

    if (rowSel) {
        GetRowSelection(ts);
        if (ts.lLength<=1) {
            long adder = -1,
                 startIndex = -1;

            if (dir) {
                adder = 1;
            }

            if (ts.lLength == 1) {
                startIndex = ts.lData[0];
            } else {
                if (dir) {
                    startIndex = -1;
                } else {
                    startIndex = verticalSpaces.lLength;
                }
            }

            startIndex += adder;
            ts.Clear();
            while ((startIndex>=0)&&(startIndex<verticalSpaces.lLength)) {
                if ((cellTypes.lData[startIndex*horizontalSpaces.lLength]&HY_TABLE_CANTSELECT)==0) {
                    ts << startIndex;
                    bool mod;
                    mod = ScrollToRow(startIndex);

                    ExpungeSelection ();
                    SetRowSelection (ts);
                    if (mod) {
                        forceUpdateForScrolling = true;
                        _MarkForUpdate();
                        forceUpdateForScrolling = false;
                    } else {
                        _MarkCellsForUpdate (ts);
                    }
                    break;
                }
                startIndex += adder;
            }
        }
    } else {
        GetSelection (ts);
        if (ts.lLength<=1) {
            long adder = 1,
                 startIndex = -1;

            switch (dir) {
            case 0: // up
                adder = -horizontalSpaces.lLength;
                break;
            case 1: // down
                adder = horizontalSpaces.lLength;
                break;
            case 2: // left
                adder = -1;
                break;
            }

            if (ts.lLength == 1) {
                startIndex = ts.lData[0];
            } else {
                switch (dir) {
                case 0: // up
                    startIndex = cellTypes.lLength;
                    break;
                case 1: // down
                    startIndex = -adder;
                    break;
                case 3: // right
                    startIndex = horizontalSpaces.lLength;
                    break;
                }
            }

            startIndex += adder;
            ts.Clear();
            while ((startIndex>=0)&&(startIndex<cellTypes.lLength)) {
                if ((cellTypes.lData[startIndex]&HY_TABLE_CANTSELECT)==0) {
                    ts << startIndex;
                    bool mod;
                    if (dir<2) {
                        mod = ScrollToRow(startIndex/horizontalSpaces.lLength);
                    } else {
                        mod = ScrollToColumn(startIndex%horizontalSpaces.lLength);
                    }
                    ExpungeSelection ();
                    SetSelection (ts,true);
                    if (mod) {
                        forceUpdateForScrolling = true;
                        _MarkForUpdate();
                        forceUpdateForScrolling = false;
                    } else {
                        _MarkCellsForUpdate (ts);
                    }
                    break;
                }
                startIndex += adder;
            }
        }
    }
}

//__________________________________________________________________
// HYHList
//__________________________________________________________________

_SimpleList*          openArrow   = nil,
                      *       closedArrow = nil;

//__________________________________________________________________

_HYHList::_HYHList (_HYRect relr,Ptr w,_List& ld, bool sing):
    _HYTable (relr,w,ld.lLength,2,20,20,HY_TABLE_STATIC_TEXT)
{
    listData.Duplicate (&ld);

    if (!openArrow) {
        openArrow = new _SimpleList ((unsigned long)3);
        checkPointer (openArrow);

        (*openArrow) << (long) ProcureIconResource(131);
        (*openArrow) << 16;
        (*openArrow) << 16;

        closedArrow = new _SimpleList ((unsigned long)3);
        checkPointer (closedArrow);

        (*closedArrow) << (long) ProcureIconResource(132);
        (*closedArrow) << 16;
        (*closedArrow) << 16;
    }

    _HYFont     df;
#ifdef __MAC__
    df.face = "Times";
    df.size = 12;
#else
#ifdef __HYPHY_GTK__
    df.face = _HY_SANS_FONT;
    df.size = 12;
#else
    df.face = "Arial";
    df.size = 14;
#endif
#endif

    df.style = HY_FONT_PLAIN;
    SetFont (df);

    for (long k=0; k<ld.lLength; k++) {
        _List * thisEntry = (_List*) listData (k);
        SetCellData (closedArrow, k, 0, HY_TABLE_ICON | HY_TABLE_CANTSELECT,true);
        SetCellData ((*thisEntry)(0), k, 1, HY_TABLE_STATIC_TEXT|HY_TABLE_BOLD ,true);
        rubrikIndex << k;
    }

    SetColumnSpacing  (1,relr.right-40-HY_SCROLLER_WIDTH,false);

    if (settings.width & HY_COMPONENT_H_SCROLL) {
        FitToHeight (relr.bottom-HY_SCROLLER_WIDTH);
    } else {
        FitToHeight (relr.bottom);
    }

    selectionType = HY_TABLE_DONT_SIZE|HY_TABLE_NO_COLS_LINES|HY_TABLE_DONT_GROW_HORIZ|HY_TABLE_DONT_GROW_VERT;
    single        = sing;
}

//__________________________________________________________________

void    _HYHList::AddRubrik (_String& rubrikName, _List& rubrikItems, long index)
{
    long        insertionPoint = 0;
    if ((index < 0) && (index >= rubrikIndex.lLength)) {
        index          = rubrikIndex.lLength;
        if (HasPadding()) {
            insertionPoint = verticalSpaces.lLength-1;
        } else {
            insertionPoint = verticalSpaces.lLength;
        }
    } else {
        insertionPoint = rubrikIndex.lData[index];
    }

    _List * newRubrik = new _List;

    checkPointer (newRubrik);

    (*newRubrik) && & rubrikName;
    (*newRubrik) && & rubrikItems;

    listData.InsertElement (newRubrik, index, false);
    rubrikIndex.InsertElement ((BaseRef)insertionPoint, index, false, false);
    DeleteObject (newRubrik);


    for (long   k = index+1; k < rubrikIndex.lLength; k++) {
        rubrikIndex.lData[k] ++;
    }


    AddRow      (insertionPoint, 20, HY_TABLE_STATIC_TEXT);
    SetCellData (closedArrow, insertionPoint, 0, HY_TABLE_ICON | HY_TABLE_CANTSELECT,true);
    SetCellData (&rubrikName, insertionPoint, 1, HY_TABLE_STATIC_TEXT|HY_TABLE_BOLD ,true);

    if (settings.width & HY_COMPONENT_H_SCROLL) {
        FitToHeight (rel.bottom-rel.top-HY_SCROLLER_WIDTH+1);
    } else {
        FitToHeight (rel.bottom-rel.top+1);
    }

    _MarkForUpdate();

}

//__________________________________________________________________

long    _HYHList::IsSingleRubrik (_SimpleList& sl)
{
    if (sl.lLength == 0) {
        return -1;
    }

    if (rubrikIndex.lLength == 0) {
        return 0;
    }

    long rIndex = 1;
    while ((sl.lData[0] >= rubrikIndex.lData[rIndex]) && (rIndex < rubrikIndex.lLength)) {
        rIndex ++;
    }

    if (rIndex == rubrikIndex.lLength) {
        sl.Offset (-rubrikIndex.lData[rIndex-1]);
        return rIndex-1;
    } else {
        long nextB = rubrikIndex.lData[rIndex],
             i = 1;

        for (; (i< sl.lLength)&&(sl.lData[i]<nextB); i++) ;

        if (i == sl.lLength) {
            sl.Offset (-rubrikIndex.lData[rIndex-1]);
            return rIndex-1;
        }
    }

    return -1;
}

//__________________________________________________________________

void    _HYHList::DeleteRubrik (long index)
{
    if ((index>=0) && (index<rubrikIndex.lLength)) {
        long delCount;

        if (index <  rubrikIndex.lLength - 1) {
            delCount = rubrikIndex.lData[index+1] - rubrikIndex.lData[index];
        } else {
            delCount = HasPadding()?verticalSpaces.lLength - rubrikIndex.lData[index]-1:verticalSpaces.lLength - rubrikIndex.lData[index];
        }

        for (long k=index+1; k<rubrikIndex.lLength; k++) {
            rubrikIndex.lData[k] -= delCount;
        }

        while (delCount) {
            DeleteRow (rubrikIndex.lData[index]);
            delCount --;
        }

        rubrikIndex.Delete (index);
        listData.Delete (index);

        if (settings.width & HY_COMPONENT_H_SCROLL) {
            FitToHeight (rel.bottom-rel.top-HY_SCROLLER_WIDTH+1);
        } else {
            FitToHeight (rel.bottom-rel.top+1);
        }
    }
    _MarkForUpdate();
}


//__________________________________________________________________

long    _HYHList::IsARubrik (long index)
{
    return rubrikIndex.Find (index);
}

//__________________________________________________________________

long    _HYHList::FindString (_String* s, long startat)
{
    for (long k=(startat>=0?startat:0); k<listData.lLength; k++) {
        _List * thisRubrik = (_List*)(*((_List*)listData(k)))(1);
        for (long j=0; j<thisRubrik->lLength; j++)
            if (s->Equal((_String*)(*thisRubrik)(j))) {
                return (k<<16)+j;
            }
    }
    return -1;
}

//__________________________________________________________________

bool    _HYHList::IsRubrikOpen (long index)
{
    if ((index>=0)&&(index<listData.lLength)) {
        if (index<listData.lLength-1) {
            return (rubrikIndex[index+1]-rubrikIndex[index]-1>0);
        } else {
            if (HasPadding()) {
                return rubrikIndex[index]<verticalSpaces.lLength-2;
            } else {
                return rubrikIndex[index]<verticalSpaces.lLength-1;
            }
        }
    }
    return false;
}

//__________________________________________________________________

long    _HYHList::RubrikIndex (long index)
{
    long result = 0;
    while ((result<rubrikIndex.lLength)&&(rubrikIndex.lData[result]<index)) {
        result ++;
    }

    if ((rubrikIndex.lData[result]>index)||(result==rubrikIndex.lLength)) {
        result--;
    }

    return (result<<16)+(index-rubrikIndex.lData[result]);
}


//__________________________________________________________________

long    _HYHList::AbsoluteIndex (long rubrik, long item)
{
    long result = 0,
         k;

    for (k=0; k<rubrik; k++) {
        _List * thisEntry = (_List*) listData (k);
        result = result + ((_List*)(*thisEntry)(1))->lLength + 1;
    }
    return result+item;
}

//__________________________________________________________________

void        _HYHList::ModifySelection (long h,long v, bool shift, bool control, bool message)
{
    if (h==0) {
        long f = IsARubrik(v);
        if (f>=0) {
            _List       * rubrikItems = (_List*) (*(_List*) listData (f))(1);
            if (IsRubrikOpen(f)) {
                SetCellData (closedArrow, v, 0, HY_TABLE_ICON | HY_TABLE_CANTSELECT,true);
                for (f=f+1; f<listData.lLength; f++) {
                    rubrikIndex.lData[f] -= rubrikItems->lLength;
                }

                for (f = v+1; f <= v+rubrikItems->lLength; f++) {
                    DeleteRow (v+1);
                }


                if (settings.width & HY_COMPONENT_H_SCROLL) {
                    FitToHeight (rel.bottom-rel.top-HY_SCROLLER_WIDTH+1);
                } else {
                    FitToHeight (rel.bottom-rel.top+1);
                }
            } else {
                for (f=f+1; f<listData.lLength; f++) {
                    rubrikIndex.lData[f] += rubrikItems->lLength;
                }

                SetCellData (openArrow, v, 0, HY_TABLE_ICON | HY_TABLE_CANTSELECT,true);

                for (f = v+1; f <= v+rubrikItems->lLength; f++) {
                    AddRow      (f,20, HY_TABLE_STATIC_TEXT);
                    SetCellData (&empty, f, 0, HY_TABLE_STATIC_TEXT | HY_TABLE_CANTSELECT,true);
                    SetCellData ((*rubrikItems)(f-v-1), f, 1, HY_TABLE_STATIC_TEXT,true);
                }

                if (settings.width & HY_COMPONENT_H_SCROLL) {
                    FitToHeight (rel.bottom-rel.top-HY_SCROLLER_WIDTH+1);
                } else {
                    FitToHeight (rel.bottom-rel.top+1);
                }
            }
            _SimpleList modRows;
            for (f=v; f<verticalSpaces.lLength; f++) {
                modRows << 2*f;
                modRows << 2*f+1;
            }
            _MarkCellsForUpdate (modRows);

            _HYTable::ModifySelection (1,v,false,false,message);
            return;
        }
    }
    _HYTable::ModifySelection (h,v,(!single)&&shift,(!single)&&control,message);
}

//__________________________________________________________________

void    _HYHList::FitToHeight (long height)
{
    bool           hasPadding = HasPadding();
    long           h = verticalSpaces.lData[verticalSpaces.lLength-(hasPadding?2:1)];

    if (hasPadding) {
        if (h<height) {
            SetRowSpacing (verticalSpaces.lLength-1,height-h-GetRowSpacing (verticalSpaces.lLength-1),false);
        } else {
            DeleteRow (verticalSpaces.lLength-1);
        }
    } else if (h<height) {
        AddRow (-1,height-h,HY_TABLE_STATIC_TEXT|HY_TABLE_CANTSELECT);
    }

    SetVisibleSize (rel);
}

//__________________________________________________________________

bool    _HYHList::HasPadding (void)
{
    if (cellTypes.lLength) {
        return (cellTypes.lData[cellTypes.lLength-1] & HY_TABLE_CANTSELECT);
    } else {
        return false;
    }
}

//__________________________________________________________________

void    _HYHList::HandleKeyMove (char dir, bool mod)
{
    _SimpleList  ts;
    GetSelection (ts);
    if (ts.lLength<=1) {
        if (dir<2) {
            _HYTable::HandleKeyMove (dir,mod);
            return;
        }

        if (ts.lLength == 1) {
            long f = IsARubrik (ts.lData[0]/2);
            if ((f>=0)&&(((dir == 3)&&(!IsRubrikOpen(f)))||((dir == 2)&&(IsRubrikOpen(f))))) {
                ModifySelection (0, ts.lData[0]/2, false, false,false);
            }
        }
    }
}
