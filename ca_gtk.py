import ca
try: import glib
except:
  try: import gobject as glib
  except: import gtk as glib

import gtk, gtk.glade, os, sys

ca.init(async=False) ;# run non-premptive
ca.admin.debug=0    ;# hide verbose
ca.admin.syncT=-2.5  ;# channels are created in the bg.
ca.quiet()        ;# redirect (most) ca anomalies to /dev/null

poll_rate_ms = 100

class throbber(object):
  def __init__(self,label):
    self.i = 0
    self.label='\r%s' % label
    self.done=False
    self.throb = ['-','\\', '|', '/']
  
  def set_label(self,label):
    self.label='\r%s' % label
    
  def throb_one(self):
    self.done=False
    sys.stderr.write(self.label+self.throb[self.i])
    sys.stderr.flush()
    self.i=(self.i+1) % 4

  def throb_done(self):
    self.done=True
    sys.stderr.write(self.label+'done')

ca_throbber=throbber('')

def ca_sleep(time_in_ms):
  iloop=glib.MainLoop()
  glib.timeout_add(time_in_ms,iloop.quit)
  iloop.run()

ca_iloop = None

def ca_mainloop():
  global ca_iloop
  ca_iloop=glib.MainLoop()
  ca_iloop.run()

def ca_quit():
  global ca_iloop
  ca_iloop.quit()

def __checkcon(cloop):
  for c in ca.admin.channels:
    if ca.admin.channels[c]['state']!='cs_conn':
      ca_throbber.throb_one()
      return True
  ca_throbber.throb_done()
  cloop.quit()
  return False
  
def ca_connectall(give_up_time=5000):
  iloop=glib.MainLoop()
  ca_throbber.set_label('connecting epics.. ')
  timeout_id=glib.timeout_add(give_up_time,iloop.quit)
  glib.timeout_add(poll_rate_ms*2,__checkcon,iloop)
  iloop.run()
  if not glib.source_remove(timeout_id):
    for c in ca.admin.channels:
      print c,ca.admin.channels[c]['state']
    raise SystemExit, 'unable to get all channels connected'

def ca_bg_task():
  try:
    ca.poll()
  except Exception, e:
    pass
  return True

ca_bg_pollId = glib.timeout_add(poll_rate_ms,ca_bg_task)

def change_ca_bg_pollrate(new_ms_rate):
  global ca_bg_pollId, poll_rate_ms
  poll_rate_ms = new_ms_rate
  glib.source_remove(ca_bg_pollId)
  ca_bg_pollId = glib.timeout_add(poll_rate_ms,ca_bg_task)

class adminWidget(object):
  
  def on_autorefresh_toggled(self,wdg):
    if wdg.get_active():
      self.stats_update()
      self.refreshId=glib.timeout_add(1000,self.stats_update)
    else:
      self.stop_refersh()
      
  def on_refresh_clicked(self,wdg):
    self.stats_update()
  
  def on_report_clicked(self,wdg):
    self.stats_update(printit=True)

  def on_debug_activate(self,wdg):
    ca.admin.debug=int(wdg.get_text())
  
  def on_pollrate_activate(self,wdg):
    change_ca_bg_pollrate(int(wdg.get_text()))
  
  def stop_refersh(self,*args):
    if self.refreshId != None:
      glib.source_remove(self.refreshId)
      self.refreshId = None
  
  def __init__(self, auto=True, path=sys.exec_prefix+'/share/python-ca', parent=None):
    
    gladefile = 'ca_admin.glade'
    
    self.glade = gtk.glade.XML(os.path.join(path,gladefile), root='top')
    self.top = self.glade.get_widget('top')
    self.glade.signal_autoconnect(self)

    if parent:
      parent.foreach(parent.remove)
      parent.add(self.top)
    
    self.tv_details=self.glade.get_widget('treeview-details')
    cell = gtk.CellRendererText()
    col = self.tv_details.insert_column_with_attributes(-1,'name',cell,text=0)
    col.set_sort_column_id(0)
    cell = gtk.CellRendererText()
    col = self.tv_details.insert_column_with_attributes(-1,'hostname',cell,text=1)
    col.set_sort_column_id(1)
    cell = gtk.CellRendererText()
    col = self.tv_details.insert_column_with_attributes(-1,'state',cell,text=2)
    col.set_sort_column_id(2)
    cell = gtk.CellRendererText()
    col = self.tv_details.insert_column_with_attributes(-1,'type',cell,text=3)
    col.set_sort_column_id(3)
    cell = gtk.CellRendererText()
    col = self.tv_details.insert_column_with_attributes(-1,'time',cell,text=4)
    col.set_sort_column_id(4)
    cell = gtk.CellRendererText()
    col = self.tv_details.insert_column_with_attributes(-1,'dim',cell,text=5)
    col.set_sort_column_id(5)
    self.tv_liststore = gtk.ListStore(str,str,str,str,str,str)
    self.tv_sm = gtk.TreeModelSort(self.tv_liststore)
    self.tv_sm.set_sort_column_id(0, gtk.SORT_ASCENDING)
    self.tv_details.set_model(self.tv_sm)
    
    self.top.connect('destroy', self.stop_refersh)

    self.glade.get_widget('version').set_text(str(ca.version))
    self.glade.get_widget('autorefresh').set_active(auto)
    self.glade.get_widget('debug').set_text('%d' % ca.admin.debug)
    self.glade.get_widget('pollrate').set_text('%d' % poll_rate_ms)
    self.glade.get_widget('syncT').set_text('%d' % ca.admin.syncT)
    self.glade.get_widget('enum_as_string').set_text('%s' % str(bool(ca.admin.enum_as_string)))

    self.refreshId=None
    
    #self.top.add_accelerator('destroy', None, ord(q), gtk.gdk.CONTROL_MASK, gtk.ACCEL_VISIBLE)
    
  def stats_update(self, printit=False):
    ch = ca.admin.channels
    cs_counts = {
      'cs_never_conn': 0,
      'cs_prev_conn': 0,
      'cs_conn': 0,
      'cs_closed': 0,
    }
    self.tv_liststore.clear()
    servers = {}
    channel_count = len(ch)
    for cn in ch:
      cs_counts[ch[cn]['state']]+=1
      try: servers[ch[cn]['hostname']] +=1
      except: servers[ch[cn]['hostname']] =1
      self.tv_liststore.append([
        str(ch[cn]['name']),
        str(ch[cn]['hostname']),
        str(ch[cn]['state']),
        str(ch[cn]['type']),
        str(ch[cn]['ts']),
        str(ch[cn]['dim']),
        ])
      if printit:
        sys.stderr.write('%-40s %-40s %-20s %-20s %-40s %-20s\n' % (
          str(ch[cn]['name']),
          str(ch[cn]['hostname']),
          str(ch[cn]['state']),
          str(ch[cn]['type']),
          str(ch[cn]['ts']),
          str(ch[cn]['dim'])
          )
        )
      self.tv_details.columns_autosize()
    
    try:
      self.glade.get_widget('channel_count').set_text('%d' % channel_count)
      for nc in cs_counts:
        self.glade.get_widget(nc).set_text('%d' % cs_counts[nc])
      sss=str(servers).replace('{','').replace('}','').replace("'",'').replace(",",'').replace(' :',':').replace(':5064','').replace('.cl.gemini.edu','')
      self.glade.get_widget('ioc_count').set_text(sss)
      keepalive = self.glade.get_widget('autorefresh').get_active()
    except:
      keepalive = False
    return keepalive

if __name__ == "__main__":
  demo=gtk.Window(type=gtk.WINDOW_TOPLEVEL)
  w = adminWidget(parent=demo)
  demo.connect('destroy', gtk.main_quit)
  demo.show()
  gtk.main()
  
  
  
