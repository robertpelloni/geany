subdir('a11y')
subdir('deprecated')
subdir('inspector')
subdir('media')
subdir('print')

bobgui_cargs = [
  '-DBOBGUI_COMPILATION',
  '-DG_LOG_DOMAIN="Bobgui"',
  '-DBOBGUI_PRINT_BACKEND_ENABLE_UNSUPPORTED',
  '-DBOBGUI_BINARY_VERSION="@0@"'.format(bobgui_binary_version),
  '-DBOBGUI_HOST="@0@"'.format(host_machine.system()),
  '-DBOBGUI_DATA_PREFIX="@0@"'.format(bobgui_prefix),
]

# List of sources that do not contain public API, and should not be
# introspected
bobgui_private_sources = files([
  'fnmatch.c',
  'gdktextureutils.c',
  'gsettings-mapping.c',
  'bobguiaccessibleattributeset.c',
  'bobguiaccessiblevaluestatic.c',
  'bobguiactionhelper.c',
  'bobguiactionmuxer.c',
  'bobguiactionobservable.c',
  'bobguiactionobserver.c',
  'bobguiallocatedbitmask.c',
  'bobguiapplicationaccels.c',
  'bobguiapplicationimpl.c',
  'bobguibookmarksmanager.c',
  'bobguibuilder-menus.c',
  'bobguibuilderprecompile.c',
  'bobguibuiltinicon.c',
  'bobguicolorplane.c',
  'bobguicolorpicker.c',
  'bobguicolorpickerkwin.c',
  'bobguicolorpickerportal.c',
  'bobguicolorpickershell.c',
  'bobguicolorscale.c',
  'bobguicolorswatch.c',
  'bobguicolumnviewcellwidget.c',
  'bobguicolumnviewrowwidget.c',
  'bobguicolumnviewtitle.c',
  'bobguiconstraintexpression.c',
  'bobguiconstraintsolver.c',
  'bobguiconstraintvflparser.c',
  'bobguicssanimatedstyle.c',
  'bobguicssanimation.c',
  'bobguicssarrayvalue.c',
  'bobguicssbgsizevalue.c',
  'bobguicssbordervalue.c',
  'bobguicsscalcvalue.c',
  'bobguicsscolor.c',
  'bobguicsscolorvalue.c',
  'bobguicsscornervalue.c',
  'bobguicsscustompropertypool.c',
  'bobguicssdimensionvalue.c',
  'bobguicssdynamic.c',
  'bobguicsseasevalue.c',
  'bobguicssenumvalue.c',
  'bobguicssfiltervalue.c',
  'bobguicssfontfeaturesvalue.c',
  'bobguicssfontvariationsvalue.c',
  'bobguicssimage.c',
  'bobguicssimageconic.c',
  'bobguicssimagecrossfade.c',
  'bobguicssimagefallback.c',
  'bobguicssimageicontheme.c',
  'bobguicssimageinvalid.c',
  'bobguicssimagelinear.c',
  'bobguicssimagepaintable.c',
  'bobguicssimageradial.c',
  'bobguicssimagerecolor.c',
  'bobguicssimagescaled.c',
  'bobguicssimageurl.c',
  'bobguicssimagevalue.c',
  'bobguicssinheritvalue.c',
  'bobguicssinitialvalue.c',
  'bobguicsskeyframes.c',
  'bobguicsslineheightvalue.c',
  'bobguicsslookup.c',
  'bobguicssmediaquery.c',
  'bobguicssnode.c',
  'bobguicssnodedeclaration.c',
  'bobguicssnodestylecache.c',
  'bobguicssnumbervalue.c',
  'bobguicsspalettevalue.c',
  'bobguicsspositionvalue.c',
  'bobguicssreferencevalue.c',
  'bobguicssrepeatvalue.c',
  'bobguicssselector.c',
  'bobguicssshadowvalue.c',
  'bobguicssshorthandproperty.c',
  'bobguicssshorthandpropertyimpl.c',
  'bobguicssstaticstyle.c',
  'bobguicssstringvalue.c',
  'bobguicssstyle.c',
  'bobguicssstylechange.c',
  'bobguicssstyleproperty.c',
  'bobguicssstylepropertyimpl.c',
  'bobguicsstransformvalue.c',
  'bobguicsstransientnode.c',
  'bobguicsstransition.c',
  'bobguicsstypes.c',
  'bobguicssunsetvalue.c',
  'bobguicssvalue.c',
  'bobguicssvariableset.c',
  'bobguicsswidgetnode.c',
  'bobguidrop.c',
  'bobguifilechooserentry.c',
  'bobguifilechoosererrorstack.c',
  'bobguifilechoosernativeportal.c',
  'bobguifilechooserutils.c',
  'bobguifilechoosercell.c',
  'bobguifilesystemmodel.c',
  'bobguifilethumbnail.c',
  'bobguifontfilter.c',
  'bobguigizmo.c',
  'bobguiiconcache.c',
  'bobguiiconcachevalidator.c',
  'bobguiiconhelper.c',
  'bobguijoinedmenu.c',
  'bobguikineticscrolling.c',
  'bobguimagnifier.c',
  'bobguimenusectionbox.c',
  'bobguimenutracker.c',
  'bobguimenutrackeritem.c',
  'bobguipanedhandle.c',
  'bobguipango.c',
  'bobguipathbar.c',
  'bobguiplacessidebar.c',
  'bobguiplacesview.c',
  'bobguiplacesviewrow.c',
  'bobguipointerfocus.c',
  'bobguipopovercontent.c',
  'bobguiprivate.c',
  'bobguiprogresstracker.c',
  'bobguirbtree.c',
  'bobguiquery.c',
  'bobguiscaler.c',
  'bobguisearchengine.c',
  'bobguisearchenginemodel.c',
  'bobguisecurememory.c',
  'bobguisizerequestcache.c',
  'bobguisortkeys.c',
  'bobguistringpair.c',
  'bobguistyleanimation.c',
  'bobguistylecascade.c',
  'bobguistyleproperty.c',
  'bobguitextencoding.c',
  'bobguitexthistory.c',
  'bobguitextviewchild.c',
  'timsort/bobguitimsort.c',
  'bobguitrashmonitor.c',
])

# List of files that contain public API, and should be introspected
bobgui_public_sources = files([
  'gskpango.c',
  'bobguiaboutdialog.c',
  'bobguiaccelgroup.c',
  'bobguiaccessible.c',
  'bobguiaccessiblerange.c',
  'bobguiaccessiblehypertext.c',
  'bobguiaccessibletext.c',
  'bobguiaccessiblevalue.c',
  'bobguiactionable.c',
  'bobguiactionbar.c',
  'bobguiadjustment.c',
  'bobguialertdialog.c',
  'bobguiapplication.c',
  'bobguiapplicationwindow.c',
  'bobguiaspectframe.c',
  'bobguiatcontext.c',
  'bobguibinlayout.c',
  'bobguibitset.c',
  'bobguiboolfilter.c',
  'bobguibookmarklist.c',
  'bobguiborder.c',
  'bobguiboxlayout.c',
  'bobguibox.c',
  'bobguibuildable.c',
  'bobguibuilder.c',
  'bobguibuilderlistitemfactory.c',
  'bobguibuilderparser.c',
  'bobguibuilderscope.c',
  'bobguibutton.c',
  'bobguicalendar.c',
  'bobguicenterbox.c',
  'bobguicenterlayout.c',
  'bobguicheckbutton.c',
  'bobguicoloreditor.c',
  'bobguicolorchooserdialog.c',
  'bobguicolorchooserwidget.c',
  'bobguicolordialog.c',
  'bobguicolordialogbutton.c',
  'bobguicolorutils.c',
  'bobguicolumnview.c',
  'bobguicolumnviewcell.c',
  'bobguicolumnviewcolumn.c',
  'bobguicolumnviewrow.c',
  'bobguicolumnviewsorter.c',
  'bobguicomposetable.c',
  'bobguiconstraintguide.c',
  'bobguiconstraintlayout.c',
  'bobguiconstraint.c',
  'bobguicssprovider.c',
  'bobguicustomfilter.c',
  'bobguicustomlayout.c',
  'bobguicustomsorter.c',
  'bobguidialogerror.c',
  'bobguidirectorylist.c',
  'bobguidragicon.c',
  'bobguidragsource.c',
  'bobguidrawingarea.c',
  'bobguidropcontrollermotion.c',
  'bobguidroptarget.c',
  'bobguidroptargetasync.c',
  'bobguidropdown.c',
  'bobguieditable.c',
  'bobguieditablelabel.c',
  'bobguiemojichooser.c',
  'bobguiemojicompletion.c',
  'bobguientry.c',
  'bobguientrybuffer.c',
  'bobguienumlist.c',
  'bobguieventcontroller.c',
  'bobguieventcontrollerfocus.c',
  'bobguieventcontrollerkey.c',
  'bobguieventcontrollerlegacy.c',
  'bobguieventcontrollermotion.c',
  'bobguieventcontrollerscroll.c',
  'bobguiexpander.c',
  'bobguiexpression.c',
  'bobguifilechooserdialog.c',
  'bobguifilechoosernative.c',
  'bobguifilechooserwidget.c',
  'bobguifiledialog.c',
  'bobguifilefilter.c',
  'bobguifilelauncher.c',
  'bobguifilter.c',
  'bobguifilterlistmodel.c',
  'bobguifixed.c',
  'bobguifixedlayout.c',
  'bobguiflattenlistmodel.c',
  'bobguiflowbox.c',
  'bobguifontchooserdialog.c',
  'bobguifontchooserutils.c',
  'bobguifontchooserwidget.c',
  'bobguifontdialog.c',
  'bobguifontdialogbutton.c',
  'bobguiframe.c',
  'bobguigesture.c',
  'bobguigesturedrag.c',
  'bobguigesturelongpress.c',
  'bobguigestureclick.c',
  'bobguigesturepan.c',
  'bobguigesturerotate.c',
  'bobguigesturesingle.c',
  'bobguigesturestylus.c',
  'bobguigestureswipe.c',
  'bobguigesturezoom.c',
  'bobguiglarea.c',
  'bobguigraphicsoffload.c',
  'bobguigrid.c',
  'bobguigridlayout.c',
  'bobguigridview.c',
  'bobguiheaderbar.c',
  'bobguiiconpaintable.c',
  'bobguiicontheme.c',
  'bobguiimage.c',
  'bobguiimagedefinition.c',
  'bobguiimcontext.c',
  'bobguiimcontextsimple.c',
  'bobguiimmodule.c',
  'bobguiimmulticontext.c',
  'bobguiinscription.c',
  'bobguilabel.c',
  'bobguilayoutchild.c',
  'bobguilayoutmanager.c',
  'bobguilevelbar.c',
  'bobguilistbase.c',
  'bobguilinkbutton.c',
  'bobguilistbox.c',
  'bobguilistfactorywidget.c',
  'bobguilistheader.c',
  'bobguilistheaderbase.c',
  'bobguilistheaderwidget.c',
  'bobguilistitem.c',
  'bobguilistitembase.c',
  'bobguilistitemfactory.c',
  'bobguilistitemmanager.c',
  'bobguilistitemwidget.c',
  'bobguilistlistmodel.c',
  'bobguilistview.c',
  'bobguimain.c',
  'bobguimaplistmodel.c',
  'bobguimediacontrols.c',
  'bobguimediafile.c',
  'bobguimediastream.c',
  'bobguimenubutton.c',
  'bobguimessagedialog.c',
  'bobguimodelbutton.c',
  'bobguimodules.c',
  'bobguimountoperation.c',
  'bobguimultifilter.c',
  'bobguimultiselection.c',
  'bobguimultisorter.c',
  'bobguinativedialog.c',
  'bobguinomediafile.c',
  'bobguinoselection.c',
  'bobguinotebook.c',
  'bobguinumericsorter.c',
  'bobguiorientable.c',
  'bobguioverlay.c',
  'bobguioverlaylayout.c',
  'bobguipadcontroller.c',
  'bobguipaned.c',
  'bobguipasswordentry.c',
  'bobguipasswordentrybuffer.c',
  'bobguipicture.c',
  'bobguipopover.c',
  'bobguipopoverbin.c',
  'bobguipopovermenu.c',
  'bobguipopovermenubar.c',
  'bobguiprintdialog.c',
  'bobguiprogressbar.c',
  'bobguipropertylookuplistmodel.c',
  'bobguirange.c',
  'bobguirecentmanager.c',
  'bobguirenderbackground.c',
  'bobguirenderborder.c',
  'bobguirendericon.c',
  'bobguirenderlayout.c',
  'bobguirendernodepaintable.c',
  'bobguirevealer.c',
  'bobguiroot.c',
  'bobguinative.c',
  'bobguiroundedbox.c',
  'bobguiscale.c',
  'bobguiscalebutton.c',
  'bobguiscrollable.c',
  'bobguiscrollbar.c',
  'bobguiscrollinfo.c',
  'bobguiscrolledwindow.c',
  'bobguisearchbar.c',
  'bobguisearchentry.c',
  'bobguisectionmodel.c',
  'bobguiselectionfiltermodel.c',
  'bobguiselectionmodel.c',
  'bobguiseparator.c',
  'bobguisettings.c',
  'bobguishortcut.c',
  'bobguishortcutaction.c',
  'bobguishortcutcontroller.c',
  'bobguishortcutmanager.c',
  'bobguishortcuttrigger.c',
  'bobguisidebarrow.c',
  'bobguisignallistitemfactory.c',
  'bobguisingleselection.c',
  'bobguisizegroup.c',
  'bobguisizerequest.c',
  'bobguislicelistmodel.c',
  'bobguisnapshot.c',
  'bobguisorter.c',
  'bobguisortlistmodel.c',
  'bobguispinbutton.c',
  'bobguispinner.c',
  'bobguistack.c',
  'bobguistacksidebar.c',
  'bobguistackswitcher.c',
  'bobguistringfilter.c',
  'bobguistringlist.c',
  'bobguistringsorter.c',
  'bobguistyleprovider.c',
  'bobguiswitch.c',
  'bobguisymbolicpaintable.c',
  'bobguitestatcontext.c',
  'bobguitestutils.c',
  'bobguitext.c',
  'bobguitextattributes.c',
  'bobguitextbtree.c',
  'bobguitextbuffer.c',
  'bobguitextchild.c',
  'bobguitexthandle.c',
  'bobguitextiter.c',
  'bobguitextlayout.c',
  'bobguitextlinedisplaycache.c',
  'bobguitextmark.c',
  'bobguitextsegment.c',
  'bobguitexttag.c',
  'bobguitexttagtable.c',
  'bobguitexttypes.c',
  'bobguitextutil.c',
  'bobguitextview.c',
  'bobguitogglebutton.c',
  'bobguitooltip.c',
  'bobguitooltipwindow.c',
  'bobguitreeexpander.c',
  'bobguitreelistmodel.c',
  'bobguitreelistrowsorter.c',
  'bobguiurilauncher.c',
  'bobguiversion.c',
  'bobguivideo.c',
  'bobguiviewport.c',
  'bobguiwidget.c',
  'bobguiwidgetfocus.c',
  'bobguiwidgetpaintable.c',
  'bobguiwindow.c',
  'bobguiwindowcontrols.c',
  'bobguiwindowgroup.c',
  'bobguiwindowhandle.c',
  'modules/visual/imgui/bobguiimgui.c',
  'modules/media/audio/bobguiaudio.c',
  'modules/network/bobguinetwork.c',
  'modules/core/data/bobguidata.c',
  'modules/media/3d/bobgui3d.c',
  'modules/media/physics/bobguiphysics.c',
  'modules/core/state/bobguistate.c',
  'modules/core/brain/bobguibrain.c',
  'modules/system/live/bobguilive.c',
  'modules/visual/dsl/bobguidsl.c',
  'modules/visual/design/bobguidesign.c',
  'modules/visual/graph/bobguigraph.c',
  'modules/network/remote/bobguiremote.c',
  'modules/core/compute/bobguicompute.c',
  'modules/media/spatial/bobguispatial.c',
  'modules/system/security/bobguisecurity.c',
  'modules/system/plugin/bobguiplugin.c',
  'modules/core/script/bobguiscript.c',
  'modules/network/sync/bobguisync.c',
  'modules/tools/test/bobguitest.c',
  'modules/media/timeline/bobguitimeline.c',
  'modules/network/cloud/bobguicloud.c',
  'modules/system/vfs/bobguivfs.c',
  'modules/network/web/bobguiweb.c',
  'modules/visual/actionregistry/bobguiactionregistry.c',
  'modules/visual/chart/bobguichart.c',
  'modules/visual/commandpalette/bobguicommandpalette.c',
  'modules/system/iot/bobguiiot.c',
  'modules/visual/dock/bobguidock.c',
  'modules/system/shell/bobguishell.c',
  'modules/core/i18n/bobguii18n.c',
  'modules/tools/report/bobguireport.c',
  'modules/system/package/bobguipackage.c',
  'modules/media/gis/bobguigis.c',
  'modules/media/shader/bobguishader.c',
  'modules/core/quantum/bobguiquantum.c',
  'modules/core/reflect/bobguireflect.c',
  'modules/core/layout/bobguilayout.c',
  'modules/tools/studio/bobguistudio.c',
  'modules/core/stream/bobguistream.c',
  'modules/media/holograph/bobguiholograph.c',
  'modules/core/meta/bobguimeta.c',
  'modules/system/vision/bobguivision.c',
  'modules/core/entity/bobguientity.c',
  'modules/visual/rhi/bobguirhi.c',
  'modules/system/input/bobguiinput.c',
  'modules/media/audio/bobguiaudiowidgets.c',
  'modules/system/ipc/bobguiipc.c',
  'modules/visual/workbench/bobguiworkbench.c',
  'modules/media/audio/bobguipluginhost.c',
  'modules/media/audio/bobguimidi2.c',
  'modules/system/os/bobguios.c',
  'modules/core/brain/bobguitranslate.c',
  'modules/core/autonomous/bobguiautonomous.c',
  'modules/system/omni/bobguiomni.c',
  'modules/core/realtime/bobguirealtime.c',
  'modules/media/bio/bobguibio.c',
  'modules/core/fin/bobguifin.c',
  'modules/network/blockchain/bobguiblockchain.c',
  'modules/tools/forge/bobguiforge.c',
])

bobgui_public_sources += bobgui_print_sources
bobgui_private_sources += print_backend_sources

bobgui_private_type_headers = files([
  'bobguicsstypesprivate.h',
  'bobguifontfilterprivate.h',
  'bobguitexthandleprivate.h',
  'bobguiplacessidebarprivate.h',
])

bobgui_public_headers = files([
  'bobguiaboutdialog.h',
  'bobguiaccelgroup.h',
  'bobguiaccessible.h',
  'bobguiaccessiblehypertext.h',
  'bobguiaccessiblerange.h',
  'bobguiaccessibletext.h',
  'bobguiactionable.h',
  'bobguiactionbar.h',
  'bobguiadjustment.h',
  'bobguialertdialog.h',
  'bobguiapplication.h',
  'bobguiapplicationwindow.h',
  'bobguiaspectframe.h',
  'bobguiatcontext.h',
  'bobguibinlayout.h',
  'bobguibitset.h',
  'bobguibookmarklist.h',
  'bobguiboolfilter.h',
  'bobguiborder.h',
  'bobguibox.h',
  'bobguiboxlayout.h',
  'bobguibuildable.h',
  'bobguibuilder.h',
  'bobguibuilderlistitemfactory.h',
  'bobguibuilderscope.h',
  'bobguibutton.h',
  'bobguicalendar.h',
  'bobguicenterbox.h',
  'bobguicenterlayout.h',
  'bobguicheckbutton.h',
  'bobguicolordialog.h',
  'bobguicolordialogbutton.h',
  'bobguicolorutils.h',
  'bobguicolumnview.h',
  'bobguicolumnviewcell.h',
  'bobguicolumnviewcolumn.h',
  'bobguicolumnviewrow.h',
  'bobguicolumnviewsorter.h',
  'bobguiconstraintguide.h',
  'bobguiconstraintlayout.h',
  'bobguiconstraint.h',
  'bobguicssprovider.h',
  'bobguicustomfilter.h',
  'bobguicustomlayout.h',
  'bobguicustomsorter.h',
  'bobguidebug.h',
  'bobguidialogerror.h',
  'bobguidirectorylist.h',
  'bobguidragicon.h',
  'bobguidragsource.h',
  'bobguidrawingarea.h',
  'bobguidropcontrollermotion.h',
  'bobguidroptarget.h',
  'bobguidroptargetasync.h',
  'bobguidropdown.h',
  'bobguieditable.h',
  'bobguieditablelabel.h',
  'bobguiemojichooser.h',
  'bobguientry.h',
  'bobguientrybuffer.h',
  'bobguienumlist.h',
  'bobguienums.h',
  'bobguieventcontroller.h',
  'bobguieventcontrollerfocus.h',
  'bobguieventcontrollerkey.h',
  'bobguieventcontrollerscroll.h',
  'bobguieventcontrollermotion.h',
  'bobguieventcontrollerlegacy.h',
  'bobguiexpander.h',
  'bobguiexpression.h',
  'bobguifiledialog.h',
  'bobguifilefilter.h',
  'bobguifilelauncher.h',
  'bobguifilter.h',
  'bobguifilterlistmodel.h',
  'bobguifixed.h',
  'bobguifixedlayout.h',
  'bobguiflattenlistmodel.h',
  'bobguiflowbox.h',
  'bobguifontdialog.h',
  'bobguifontdialogbutton.h',
  'bobguiframe.h',
  'bobguigesture.h',
  'bobguigesturedrag.h',
  'bobguigesturelongpress.h',
  'bobguigestureclick.h',
  'bobguigesturepan.h',
  'bobguigesturerotate.h',
  'bobguigesturesingle.h',
  'bobguigesturestylus.h',
  'bobguigestureswipe.h',
  'bobguigesturezoom.h',
  'bobguiglarea.h',
  'bobguigraphicsoffload.h',
  'bobguigrid.h',
  'bobguigridlayout.h',
  'bobguigridview.h',
  'bobguiheaderbar.h',
  'bobguiicontheme.h',
  'bobguiiconpaintable.h',
  'bobguiimage.h',
  'bobguiimcontext.h',
  'bobguiimcontextsimple.h',
  'bobguiimmodule.h',
  'bobguiimmulticontext.h',
  'bobguiinscription.h',
  'bobguilabel.h',
  'bobguilayoutchild.h',
  'bobguilayoutmanager.h',
  'bobguilevelbar.h',
  'bobguilinkbutton.h',
  'bobguilistbase.h',
  'bobguilistbox.h',
  'bobguilistheader.h',
  'bobguilistitem.h',
  'bobguilistitemfactory.h',
  'bobguilistview.h',
  'bobguimain.h',
  'bobguimaplistmodel.h',
  'bobguimediacontrols.h',
  'bobguimediafile.h',
  'bobguimediastream.h',
  'bobguimenubutton.h',
  'bobguimountoperation.h',
  'bobguimultifilter.h',
  'bobguimultiselection.h',
  'bobguimultisorter.h',
  'bobguinative.h',
  'bobguinativedialog.h',
  'bobguinoselection.h',
  'bobguinotebook.h',
  'bobguinumericsorter.h',
  'bobguiorientable.h',
  'bobguioverlay.h',
  'bobguioverlaylayout.h',
  'bobguipadcontroller.h',
  'bobguipaned.h',
  'bobguipasswordentry.h',
  'bobguipasswordentrybuffer.h',
  'bobguipicture.h',
  'bobguipopover.h',
  'bobguipopoverbin.h',
  'bobguipopovermenu.h',
  'bobguipopovermenubar.h',
  'bobguiprintdialog.h',
  'bobguiprogressbar.h',
  'bobguirange.h',
  'bobguirecentmanager.h',
  'bobguirevealer.h',
  'bobguiroot.h',
  'bobguiscale.h',
  'bobguiscalebutton.h',
  'bobguiscrollable.h',
  'bobguiscrollbar.h',
  'bobguiscrollinfo.h',
  'bobguiscrolledwindow.h',
  'bobguisearchbar.h',
  'bobguisearchentry.h',
  'bobguisectionmodel.h',
  'bobguiselectionfiltermodel.h',
  'bobguiselectionmodel.h',
  'bobguiseparator.h',
  'bobguisettings.h',
  'bobguishortcut.h',
  'bobguishortcutaction.h',
  'bobguishortcutcontroller.h',
  'bobguishortcutmanager.h',
  'bobguishortcuttrigger.h',
  'bobguisignallistitemfactory.h',
  'bobguisingleselection.h',
  'bobguisizegroup.h',
  'bobguisizerequest.h',
  'bobguislicelistmodel.h',
  'bobguisnapshot.h',
  'bobguisorter.h',
  'bobguisortlistmodel.h',
  'bobguispinbutton.h',
  'bobguispinner.h',
  'bobguistack.h',
  'bobguistacksidebar.h',
  'bobguistackswitcher.h',
  'bobguistringfilter.h',
  'bobguistringlist.h',
  'bobguistringsorter.h',
  'bobguistyleprovider.h',
  'bobguiswitch.h',
  'bobguisymbolicpaintable.h',
  'bobguitestatcontext.h',
  'bobguitestutils.h',
  'bobguitext.h',
  'bobguitextbuffer.h',
  'bobguitextchild.h',
  'bobguitextiter.h',
  'bobguitextmark.h',
  'bobguitexttag.h',
  'bobguitexttagtable.h',
  'bobguitextview.h',
  'bobguitogglebutton.h',
  'bobguitooltip.h',
  'bobguitreeexpander.h',
  'bobguitreelistmodel.h',
  'bobguitreelistrowsorter.h',
  'bobguitypes.h',
  'bobguiurilauncher.h',
  'bobguivideo.h',
  'bobguiviewport.h',
  'bobguiwidget.h',
  'bobguiwidgetpaintable.h',
  'bobguiwindow.h',
  'bobguiwindowcontrols.h',
  'bobguiwindowgroup.h',
  'bobguiwindowhandle.h',
  'modules/visual/imgui/bobguiimgui.h',
  'modules/media/audio/bobguiaudio.h',
  'modules/network/bobguinetwork.h',
  'modules/core/data/bobguidata.h',
  'modules/media/3d/bobgui3d.h',
  'modules/media/physics/bobguiphysics.h',
  'modules/core/state/bobguistate.h',
  'modules/core/brain/bobguibrain.h',
  'modules/system/live/bobguilive.h',
  'modules/visual/dsl/bobguidsl.h',
  'modules/visual/design/bobguidesign.h',
  'modules/visual/graph/bobguigraph.h',
  'modules/network/remote/bobguiremote.h',
  'modules/core/compute/bobguicompute.h',
  'modules/media/spatial/bobguispatial.h',
  'modules/system/security/bobguisecurity.h',
  'modules/system/plugin/bobguiplugin.h',
  'modules/core/script/bobguiscript.h',
  'modules/network/sync/bobguisync.h',
  'modules/tools/test/bobguitest.h',
  'modules/media/timeline/bobguitimeline.h',
  'modules/network/cloud/bobguicloud.h',
  'modules/system/vfs/bobguivfs.h',
  'modules/network/web/bobguiweb.h',
  'modules/visual/actionregistry/bobguiactionregistry.h',
  'modules/visual/chart/bobguichart.h',
  'modules/visual/commandpalette/bobguicommandpalette.h',
  'modules/system/iot/bobguiiot.h',
  'modules/visual/dock/bobguidock.h',
  'modules/system/shell/bobguishell.h',
  'modules/core/i18n/bobguii18n.h',
  'modules/tools/report/bobguireport.h',
  'modules/system/package/bobguipackage.h',
  'modules/media/gis/bobguigis.h',
  'modules/media/shader/bobguishader.h',
  'modules/core/quantum/bobguiquantum.h',
  'modules/core/reflect/bobguireflect.h',
  'modules/core/layout/bobguilayout.h',
  'modules/tools/studio/bobguistudio.h',
  'modules/core/stream/bobguistream.h',
  'modules/media/holograph/bobguiholograph.h',
  'modules/core/meta/bobguimeta.h',
  'modules/system/vision/bobguivision.h',
  'modules/core/entity/bobguientity.h',
  'modules/visual/rhi/bobguirhi.h',
  'modules/system/input/bobguiinput.h',
  'modules/media/audio/bobguiaudiowidgets.h',
  'modules/system/ipc/bobguiipc.h',
  'modules/visual/workbench/bobguiworkbench.h',
  'modules/media/audio/bobguipluginhost.h',
  'modules/media/audio/bobguimidi2.h',
  'modules/system/os/bobguios.h',
  'modules/core/brain/bobguitranslate.h',
  'modules/core/autonomous/bobguiautonomous.h',
  'modules/system/omni/bobguiomni.h',
  'modules/core/realtime/bobguirealtime.h',
  'modules/media/bio/bobguibio.h',
  'modules/core/fin/bobguifin.h',
  'modules/network/blockchain/bobguiblockchain.h',
  'modules/tools/forge/bobguiforge.h',
  'modules/core/core.h',
  'modules/media/media.h',
  'modules/network/network.h',
  'modules/system/system.h',
  'modules/tools/tools.h',
  'modules/visual/visual.h',
  'modules/bobguimodules.h',
  'bobgui.h',
])

install_headers(bobgui_public_headers, subdir: 'bobgui-4.0/bobgui/')

bobgui_cpp_headers = files([
  'cpp/action_registry.hpp',
  'cpp/animation.hpp',
  'cpp/app_shell.hpp',
  'cpp/application.hpp',
  'cpp/bobgui.hpp',
  'cpp/canvas.hpp',
  'cpp/command_palette.hpp',
  'cpp/core.hpp',
  'cpp/dashboard_shell.hpp',
  'cpp/database.hpp',
  'cpp/document_shell.hpp',
  'cpp/dock_manager.hpp',
  'cpp/file_system.hpp',
  'cpp/graphics.hpp',
  'cpp/ipc.hpp',
  'cpp/layout.hpp',
  'cpp/module.hpp',
  'cpp/module/core.hpp',
  'cpp/module/media.hpp',
  'cpp/module/network.hpp',
  'cpp/module/system.hpp',
  'cpp/module/tools.hpp',
  'cpp/module/visual.hpp',
  'cpp/network.hpp',
  'cpp/network_pillar.hpp',
  'cpp/object_handle.hpp',
  'cpp/property.hpp',
  'cpp/resource.hpp',
  'cpp/settings.hpp',
  'cpp/signal.hpp',
  'cpp/studio_shell.hpp',
  'cpp/system.hpp',
  'cpp/theme.hpp',
  'cpp/timer.hpp',
  'cpp/tool_surface.hpp',
  'cpp/tool_surface_builder.hpp',
  'cpp/toolbar_builder.hpp',
  'cpp/visual_pillar.hpp',
  'cpp/widget.hpp',
  'cpp/workbench.hpp',
])
install_headers(bobgui_cpp_headers, subdir: 'bobgui-4.0/bobgui/cpp')

install_headers(bobgui_deprecated_headers, subdir: 'bobgui-4.0/bobgui/deprecated')
install_headers(bobgui_print_headers, subdir: 'bobgui-4.0/bobgui/print')

bobgui_sources = bobgui_public_sources + bobgui_private_sources

if os_unix and tracker3_dep.found()
  bobgui_sources += 'bobguisearchenginetracker3.c'
endif

bobgui_unix_print_main_header = [
  'bobguiunixprint.h'
]

if os_unix
  install_headers(bobgui_unix_print_main_header, subdir: 'bobgui-4.0/unix-print/bobgui')
  install_headers(bobgui_unix_print_headers, subdir: 'bobgui-4.0/unix-print/bobgui/print')
  bobgui_sources += bobgui_unix_print_sources
endif

if os_win32
  bobgui_sources += bobgui_win32_print_sources
  bobgui_sources += [
    'bobguiapplication-win32.c',
    'bobguicolorpickerwin32.c',
    'bobguiimcontextime.c',
    'bobguifilechoosernativewin32.c',
    'bobguiwin32.c',
    'bobguishowwin32.c',
  ]
endif

if x11_enabled
  bobgui_sources += ['bobguiapplication-x11.c', 'bobguimountoperation-x11.c']
endif

if wayland_enabled
  xml = wlmod.find_protocol('text-input', state: 'unstable', version: 3)
  bobgui_sources += [
    wlmod.scan_xml(xml),
    'bobguiimcontextwayland.c',
    'bobguiapplication-wayland.c',
  ]
endif

if x11_enabled or wayland_enabled
  bobgui_sources += ['bobguiapplication-dbus.c']
endif

if win32_enabled
  bobgui_sources += ['bobguimountoperation-stub.c']
endif

if macos_enabled
  bobgui_sources += files([
    'bobguisearchenginequartz.c',
    'bobguiapplication-quartz.c',
    'bobguiapplication-quartz-menu.c',
    'bobguicolorpickerquartz.c',
    'bobguifilechoosernativequartz.c',
    'bobguiimcontextquartz.c',
    'bobguiquartz.c',
    'bobguiwindowbuttonsquartz.c',
  ])

  bobgui_cargs += ['-xobjective-c']  # FIXME? maybe add_languages() instead?
endif

if android_enabled
  bobgui_sources += files([
    'bobguiapplication-android.c',
    'bobguifilechoosernativeandroid.c',
    'bobguiimcontextandroid.c',
  ])
endif

if not (x11_enabled or win32_enabled)
  bobgui_sources += ['bobguimountoperation-stub.c', ]
endif

if not os_win32
  bobgui_sources += ['bobguiopenuriportal.c', ]
endif

gen_bobgui_gresources_xml = find_program('gen-bobgui-gresources-xml.py')
bobgui_gresources_xml = configure_file(output: 'bobgui.gresources.xml',
  command: [
    gen_bobgui_gresources_xml,
    meson.current_source_dir(),
    host_machine.endian(),
    '@OUTPUT@'
  ],
)

theme_deps = []
# For git checkouts, but not for tarballs...
if not fs.exists('theme/Default/Default-light.css')
  # ... build the theme files
  sassc = find_program('sassc', native: true)
  sassc_opts = [ '-a', '-M', '-t', 'compact' ]

  subdir('theme/Default')

  theme_deps += [
    default_theme_deps,
  ]
endif

bobguiresources = gnome.compile_resources('bobguiresources',
  bobgui_gresources_xml,
  dependencies: theme_deps,
  source_dir: [
    # List in order of preference
    meson.current_build_dir(),
    meson.current_source_dir(),
  ],
  c_name: '_bobgui',
  extra_args: '--manual-register',
)

foreach lang : [
    'bn',
    'de',
    'da',
    'fr',
    'es',
    'et',
    'fi',
    'hi',
    'hu',
    'it',
    'ja',
    'ko',
    'lt',
    'ms',
    'nb',
    'nl',
    'pl',
    'pt',
    'ru',
    'sv',
    'th',
    'uk',
    'zh'
  ]
  conf = configuration_data()
  conf.set('lang', lang)
  resxml = configure_file(input: 'emoji/gresource.xml.in',
    output: lang + '.gresource.xml',
    configuration: conf
  )

  gnome.compile_resources(lang,
    resxml,
    source_dir: meson.current_source_dir() / 'emoji',
    gresource_bundle: true,
    install: true,
    install_dir: bobgui_datadir / 'bobgui-4.0/emoji',
  )
endforeach

bobgui_x11_sources = files([
  'bobguiapplication-x11.c',
  'bobguimountoperation-x11.c',
])

bobgui_wayland_sources = files([
  'bobguiapplication-wayland.c'
])

bobgui_use_wayland_or_x11_c_sources = files([
  'bobguiapplication-dbus.c'
])

bobgui_dbus_src = gnome.gdbus_codegen('bobguidbusgenerated',
  sources:'bobguidbusinterfaces.xml',
  interface_prefix: 'org.Bobgui.',
  namespace: '_Bobgui',
)

bobguimarshalers = gnome.genmarshal('bobguimarshalers',
  sources: 'bobguimarshalers.list',
  prefix: '_bobgui_marshal',
  valist_marshallers: true,
)

bobguimarshal_h = bobguimarshalers[1]

bobguitypebuiltins = gnome.mkenums('bobguitypebuiltins',
  sources: bobgui_public_headers + bobgui_deprecated_headers + bobgui_print_headers,
  c_template: 'bobguitypebuiltins.c.template',
  h_template: 'bobguitypebuiltins.h.template',
  install_dir: bobgui_includedir / 'bobgui-4.0/bobgui',
  install_header: true,
)

bobguitypebuiltins_h = bobguitypebuiltins[1]

bobguiprivatetypebuiltins = gnome.mkenums('bobguiprivatetypebuiltins',
  sources: bobgui_private_type_headers,
  c_template: 'bobguiprivatetypebuiltins.c.template',
  h_template: 'bobguiprivatetypebuiltins.h.template',
)

bobguiprivatetypebuiltins_h = bobguiprivatetypebuiltins[1]

# Generate bobguitypefuncs.inc
typefuncs = custom_target('bobguitypefuncs.inc',
  depends: [ gdkenum_h, gskenum_h, ],
  output: 'bobguitypefuncs.inc',
  input: gdk_headers +
    gsk_public_headers +
    bobgui_public_headers +
    bobgui_deprecated_headers +
    [ bobguitypebuiltins_h, ] +
    [ bobgui_css_enum_h, gdkenum_h, gskenum_h, ],
  command: [
    find_program('gentypefuncs.py'),
    '@OUTPUT@',
    '@INPUT@',
  ],
  install: false,
)

if os_win32
  xdp_dbus_generated = []
else
  xdp_dbus_generated = gnome.gdbus_codegen('xdp-dbus',
    sources : 'org.freedesktop.portal.OpenURI.xml',
    interface_prefix : 'org.freedesktop.portal.',
    namespace : 'BobguiXdp',
  )
endif

bobguiconfig_cdata = configuration_data()
bobguiconfig_cdata.set('BOBGUI_ACCESSIBILITY_ATSPI', bobgui_a11y_backends.contains('atspi'))

bobguiconfig = configure_file(
  input: 'bobguiconfig.h.meson',
  output: 'bobguiconfig.h',
  configuration: bobguiconfig_cdata,
  install_dir: bobgui_includedir / 'bobgui-4.0/bobgui',
)

bobguiversion_cdata = configuration_data()
bobguiversion_cdata.set('BOBGUI_MAJOR_VERSION', bobgui_major_version)
bobguiversion_cdata.set('BOBGUI_MINOR_VERSION', bobgui_minor_version)
bobguiversion_cdata.set('BOBGUI_MICRO_VERSION', bobgui_micro_version)
bobguiversion_cdata.set('BOBGUI_BINARY_AGE', bobgui_binary_age)
bobguiversion_cdata.set('BOBGUI_INTERFACE_AGE', bobgui_interface_age)
bobguiversion_cdata.set('BOBGUI_VERSION', bobgui_version)
bobguiversion_cdata.set('BOBGUI_API_VERSION', bobgui_api_version)

if host_machine.system() == 'windows'
  v = bobgui_binary_version.split('.')
  bobgui_manifest_version = '.'.join(v.get(0, '0'), v.get(1, '0'), v.get(2, '0'), v.get(3, '0'))
  bobguiversion_cdata.set('BOBGUI_MANIFEST_VERSION', bobgui_manifest_version)
endif

bobguiversion = configure_file(input: 'bobguiversion.h.in',
  output: 'bobguiversion.h',
  configuration: bobguiversion_cdata,
  install: true,
  install_dir: bobgui_includedir / 'bobgui-4.0/bobgui',
)

bobgui_gen_headers = [
  bobguimarshal_h,
  bobguitypebuiltins_h,
  bobguiprivatetypebuiltins_h,
  bobguiconfig,
  bobguiversion,
]

font_script_language_sources = files([
  'language-names.c',
])

bobgui_sources += [
  bobgui_a11y_src,
  bobgui_dbus_src,
  bobgui_deprecated_sources,
  inspector_sources,
  media_sources,
  bobguiresources,
  bobguimarshalers,
  bobguiprivatetypebuiltins,
  bobguitypebuiltins,
  font_script_language_sources,
]

bobgui_deps = [
  gmodule_dep,
  gobject_dep,
  platform_gio_dep,
  pangocairo_dep,
  harfbuzz_dep,
  hb_subset_dep,
  fribidi_dep,
  cairogobj_dep,
  fontconfig_dep,
  pixbuf_dep,
  epoxy_dep,
  media_deps,
  libm,
  graphene_dep,
  accesskit_dep,
  profile_conf_h,
]

if x11_enabled
  x11_data_prefix = dependency('x11').get_variable(pkgconfig: 'prefix')

  bobgui_cargs += [ '-DX11_DATA_PREFIX="@0@"'.format(x11_data_prefix), ]
  bobgui_sources += bobgui_x11_sources
  bobgui_deps += [ xi_dep, x11_dep, ]
endif

if wayland_enabled
  bobgui_sources += bobgui_wayland_sources
endif

if broadway_enabled
  bobgui_sources += ['bobguiimcontextbroadway.c']
endif

if win32_enabled
  bobgui_cargs += ['-DBOBGUI_PRINT_BACKENDS="file,lpr"']
  bobgui_deps += [ giowin32_dep, pangowin32_dep ]

  bobgui_deps += [
    cc.find_library('advapi32'),
    cc.find_library('comctl32'),
    cc.find_library('crypt32'), # For CryptProtectMemory()
    cc.find_library('dwmapi'),
    cc.find_library('imm32'),
    cc.find_library('setupapi'),
    cc.find_library('winmm'),
  ]

  bobguiwin32rc = configure_file(input: 'bobgui-win32.rc.body.in',
    output: 'bobgui-win32.rc.body',
    configuration: bobguiversion_cdata,
  )

  win32_manifest = configure_file(input: 'libbobgui.manifest.in',
    output: 'libbobgui.manifest',
    configuration: bobguiversion_cdata,
  )

  win32res = import('windows').compile_resources(bobguiwin32rc, include_directories: win32rcinc)
  bobgui_sources += win32res
endif

# So we don't add these twice
if x11_enabled or wayland_enabled
  bobgui_sources += bobgui_use_wayland_or_x11_c_sources
endif

if pangoft_dep.found()
  bobgui_deps += pangoft_dep
endif

if macos_enabled
  bobgui_macos_frameworks = [
    'AppKit',
  ]
  bobgui_deps += [dependency('appleframeworks', modules: bobgui_macos_frameworks)]
endif

bobgui_deps += cloudproviders_dep

if os_unix
  bobgui_deps += tracker3_dep
endif

# Unconditional. If libintl isn't found,
# the object just does nothing being in the deplist
bobgui_deps += libintl_dep

if os_linux
  install_data('bobgui-mime.xml', install_dir: join_paths (bobgui_datadir, 'mime/packages'), install_tag: 'runtime')
endif

bobgui_settings_schemas = [
  'org.bobgui.bobgui4.Settings.FileChooser.gschema.xml',
  'org.bobgui.bobgui4.Settings.ColorChooser.gschema.xml',
  'org.bobgui.bobgui4.Settings.EmojiChooser.gschema.xml',
  'org.bobgui.bobgui4.Settings.Debug.gschema.xml',
  'org.bobgui.bobgui4.Inspector.gschema.xml',
]

install_data(bobgui_settings_schemas, install_dir: bobgui_schemasdir, install_tag: 'runtime')
gnome.compile_schemas(depend_files: files(bobgui_settings_schemas), build_by_default: true)
bobgui_schema_build_dir = meson.current_build_dir()

# Check for more things
if cc.has_header('langinfo.h')
  foreach nl_enum: [ '_NL_MEASUREMENT_MEASUREMENT',
                     '_NL_PAPER_HEIGHT',
                     '_NL_PAPER_WIDTH',
                     '_NL_TIME_FIRST_WEEKDAY', ]
    cdata.set('HAVE_' + nl_enum, cc.has_header_symbol('langinfo.h', nl_enum))
  endforeach
endif

# Generate versions for macOS dylib
darwin_versions = [
  # compatibility version
  1 + '@0@'.format(bobgui_binary_age - bobgui_interface_age).to_int(),
  # current version
  '@0@.@1@'.format(1 + '@0@'.format(bobgui_binary_age - bobgui_interface_age).to_int(), bobgui_interface_age),
]

# this needs to happen after mkenums (since the svg code uses bobgui enum types)
subdir('svg')

# Library
libbobgui_static = static_library('bobgui',
  sources: [typefuncs, bobgui_sources, bobguimarshal_h, bobguiprivatetypebuiltins_h, xdp_dbus_generated],
  c_args: bobgui_cargs + print_backend_cflags + common_cflags,
  include_directories: [confinc, gdkinc, gskinc, bobguiinc],
  dependencies: bobgui_deps + [libbobgui_css_dep, libbobgui_svg_dep, libgdk_dep, libgsk_dep, media_deps, print_backend_deps],
  link_with: [libbobgui_css, libbobgui_svg, libgdk, libgsk ],
)

libbobgui = shared_library('bobgui-4',
  c_args: bobgui_cargs + common_cflags,
  include_directories: [confinc, gdkinc, gskinc, bobguiinc],
  dependencies: bobgui_deps + [libbobgui_css_dep, libbobgui_svg_dep, libgdk_dep, libgsk_dep],
  link_whole: [libbobgui_static, libbobgui_css, libbobgui_svg, libgdk, libgsk ],
  link_args: common_ldflags,
  soversion: bobgui_soversion,
  version: bobgui_library_version,
  darwin_versions: darwin_versions,
  gnu_symbol_visibility: 'hidden',
  install: true,
)

bobgui_dep_sources = [bobguiconfig, bobguiversion, bobguitypebuiltins_h]

# Introspection
if build_gir
  gir_args = [
    '-DBOBGUI_COMPILATION',
    '--quiet',
     '--doc-format=gi-docgen',
  ]

  gdk_gir_inc = [ 'cairo-1.0', 'Gio-2.0', 'GdkPixbuf-2.0', 'Pango-1.0', 'PangoCairo-1.0' ]

  gdk_gir = gnome.generate_gir(libbobgui,
    sources: gdk_public_headers + gdk_deprecated_headers + gdk_public_sources + gdk_deprecated_sources + [ gdkenum_h ],
    namespace: 'Gdk',
    nsversion: bobgui_api_version,
    identifier_prefix: 'Gdk',
    symbol_prefix: 'gdk',
    export_packages: 'bobgui4',
    includes: gdk_gir_inc,
    header: 'gdk/gdk.h',
    install: true,
    dependencies: libgdk_dep,
    extra_args: gir_args,
    fatal_warnings: false,
  )
  bobgui_dep_sources += gdk_gir

  if x11_enabled
    gdk_x11_gir = gnome.generate_gir(libbobgui,
      sources: gdk_x11_public_headers + gdk_x11_public_sources,
      namespace: 'GdkX11',
      nsversion: bobgui_api_version,
      identifier_prefix: 'Gdk',
      symbol_prefix: 'gdk',
      export_packages: 'bobgui4-x11',
      includes: [ gdk_gir[0], 'xlib-2.0', ],
      install: true,
      dependencies: libgdk_dep,
      header: 'gdk/x11/gdkx.h',
      extra_args: gir_args,
      fatal_warnings: get_option('werror'),
    )
    bobgui_dep_sources += gdk_x11_gir
  endif

  if wayland_enabled
    gdk_wayland_gir = gnome.generate_gir(libbobgui,
      sources: gdk_wayland_public_headers + gdk_wayland_sources,
      namespace: 'GdkWayland',
      nsversion: bobgui_api_version,
      identifier_prefix: 'Gdk',
      symbol_prefix: 'gdk',
      export_packages: 'bobgui4-wayland',
      includes: [ gdk_gir[0], ],
      install: true,
      dependencies: libgdk_dep,
      header: 'gdk/wayland/gdkwayland.h',
      extra_args: gir_args,
      fatal_warnings: get_option('werror'),
    )
    bobgui_dep_sources += gdk_wayland_gir
  endif

  if win32_enabled
    gdk_win32_gir = gnome.generate_gir(libbobgui,
      sources: gdk_win32_public_headers + gdk_win32_public_sources,
      namespace: 'GdkWin32',
      nsversion: bobgui_api_version,
      identifier_prefix: 'Gdk',
      symbol_prefix: 'gdk',
      export_packages: 'bobgui4-win32',
      includes: [ gdk_gir[0], 'win32-1.0', ],
      install: true,
      dependencies: libgdk_dep,
      header: 'gdk/win32/gdkwin32.h',
      extra_args: gir_args,
      # https://gitlab.gnome.org/GNOME/gobject-introspection/-/issues/574
      fatal_warnings: false,
    )
    bobgui_dep_sources += gdk_win32_gir
  endif

  if macos_enabled
    gdk_macos_gir = gnome.generate_gir(libbobgui,
      sources: gdk_macos_public_headers + gdk_macos_public_sources,
      namespace: 'GdkMacos',
      nsversion: bobgui_api_version,
      identifier_prefix: 'Gdk',
      symbol_prefix: 'gdk',
      export_packages: 'bobgui4-macos',
      includes: [ gdk_gir[0], ],
      install: true,
      dependencies: libgdk_dep,
      header: 'gdk/macos/gdkmacos.h',
      extra_args: gir_args,
      fatal_warnings: get_option('werror'),
    )
    bobgui_dep_sources += gdk_macos_gir
  endif

  if android_enabled
    gdk_android_gir = gnome.generate_gir(libbobgui,
      sources: gdk_android_public_headers + gdk_android_sources,
      namespace: 'GdkAndroid',
      nsversion: bobgui_api_version,
      identifier_prefix: 'Gdk',
      symbol_prefix: 'gdk',
      export_packages: 'bobgui4-android',
      includes: [ gdk_gir[0], ],
      install: true,
      dependencies: libgdk_dep,
      header: 'gdk/android/gdkandroid.h',
      extra_args: gir_args,
      fatal_warnings: get_option('werror'),
    )
    bobgui_dep_sources += gdk_android_gir
  endif

  gsk_gir_inc = [ gdk_gir[0], 'Graphene-1.0' ]

  gsk_gir = gnome.generate_gir(libbobgui,
    sources: gsk_public_headers + gsk_public_sources + [ gskenum_h ],
    namespace: 'Gsk',
    nsversion: bobgui_api_version,
    identifier_prefix: 'Gsk',
    symbol_prefix: 'gsk',
    export_packages: 'bobgui4',
    includes: gsk_gir_inc,
    header: 'gsk/gsk.h',
    install: true,
    dependencies: libgsk_dep,
    extra_args: gir_args,
    fatal_warnings: false,
  )
  bobgui_dep_sources += gsk_gir

  bobgui_introspection_sources = [
    bobgui_public_headers,
    bobgui_public_sources,
    bobgui_print_headers,
    bobgui_css_public_headers,
    bobgui_css_public_sources,
    bobgui_svg_enum_h,
    bobgui_svg_public_headers,
    bobgui_svg_public_sources,
    bobgui_deprecated_headers,
    bobgui_deprecated_sources,
    bobguitypebuiltins_h,
    bobguiversion,
    bobguiconfig,
  ]

  if os_unix
    bobgui_introspection_sources += [ bobgui_unix_print_headers + bobgui_unix_print_main_header, bobgui_unix_print_sources ]
  endif

  if os_win32
    bobgui_introspection_sources += [ bobgui_win32_print_headers, bobgui_win32_print_sources ]
  endif

  bobgui_gir = gnome.generate_gir(libbobgui,
    sources: bobgui_introspection_sources,
    namespace: 'Bobgui',
    nsversion: bobgui_api_version,
    identifier_prefix: 'Bobgui',
    symbol_prefix: 'bobgui',
    export_packages: 'bobgui4',
    includes: [ gdk_gir[0], gsk_gir[0], ],
    header: 'bobgui/bobgui.h',
    install: true,
    dependencies: [bobgui_deps, libgsk_dep],
    extra_args: gir_args,
    fatal_warnings: false,
  )
  bobgui_dep_sources += bobgui_gir
endif

libbobgui_dep = declare_dependency(sources: bobgui_dep_sources,
  include_directories: [confinc, bobguiinc],
  dependencies: bobgui_deps + [libbobgui_css_dep, libbobgui_svg_dep, libgdk_dep, libgsk_dep],
  link_with: libbobgui,
  link_args: common_ldflags,
)

libbobgui_static_dep = declare_dependency(sources: bobgui_dep_sources,
  include_directories: [confinc, bobguiinc],
  dependencies: bobgui_deps + [libbobgui_css_dep, libbobgui_svg_dep, libgdk_dep, libgsk_dep],
  link_with: [libbobgui_static, libbobgui_css, libgdk, libgsk ],
  link_args: common_ldflags,
)

subdir('compose')
