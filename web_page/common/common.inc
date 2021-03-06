<!--- common stuff --->

<set CMT="!--">


<!--- news items --->

<!---
  [BeginNews news_title="xxx" news_date="xxx"]
  ...body text...
  [EndNews]
--->

<block BeginNews>
<div class="news">
  <table class="news_title" width="95%">
  <tr>
    <td align="left">  <use title> </td>
    <td align="right"> <use date> </td>
  </tr>
  </table>
  <div class="news_body">
</block>
 
<block BigNews>
<div class="news">
  <table class="news_big_title" width="95%">
  <tr>
    <td align="left">  <use title> </td>
    <td align="right"> <use date> </td>
  </tr>
  </table>
  <div class="news_body">
</block>
 
<block HugeNews>
<div class="news">
  <table class="news_huge_title" width="95%">
  <tr>
    <td align="left">  <use title> </td>
    <td align="right"> <use date> </td>
  </tr>
  </table>
  <div class="news_body">
</block>
 
<block EndNews>
  </div>
</div>
</block>


<!--- DDF stuff --->

<!---
  Synopsis macro
  ==============
  file  : filename of DDF file
  lump  : WAD lump name
  tag   : tag name
--->

<block synopsis>

<div>
<pre class="synopsis">
Filename : <use file>
Wad lump : <use lump>
Data tag : &lt;<use tag>&gt;
</pre>
</div>

</block>


<!--- RTS stuff --->


<!--- Screenshot stuff --->

<!--- 
  Thumb macro
  ===========

  file  : filename of image (with extension, but no path)
  dest  : filename of dest page (ext, no path)
  title : title for the thumbnail image
--->

<block thumb>
  <table class="thumb_tab">
    <caption> <use title> </caption>
    <tr><td>
    <a class="thumb_link" href="shots/${dest}">
      <img class="thumb_image" src="thumbs/${file}"
       width="160" height="120" alt="Screenshot">
    </a>
  </td></tr>
  </table>
</block>

<block thumb_row>
  <table class="thumb_tab">
    <caption> <use title> </caption>
    <tr>
      <td>
        <a class="thumb_link" href="shots/${base}_A.htm">
          <img class="thumb_image" src="thumbs/${base}_TA.png"
           width="160" height="120" alt="Screenshot">
        </a>
      </td>
      <td>
        <a class="thumb_link" href="shots/${base}_B.htm">
          <img class="thumb_image" src="thumbs/${base}_TB.png"
           width="160" height="120" alt="Screenshot">
        </a>
      </td>
      <td>
        <a class="thumb_link" href="shots/${base}_C.htm">
          <img class="thumb_image" src="thumbs/${base}_TC.png"
           width="160" height="120" alt="Screenshot">
        </a>
      </td>
    </tr>
  </table>
</block>

<block thumb_mega>
  <table class="thumb_tab">
    <caption> <use title> </caption>
    <tr>
      <td>
        <a class="thumb_link" href="shots/${base}_A.htm">
          <img class="thumb_image" src="thumbs/${base}_TA.png"
           width="160" height="120" alt="Screenshot">
        </a>
      </td>
      <td>
        <a class="thumb_link" href="shots/${base}_B.htm">
          <img class="thumb_image" src="thumbs/${base}_TB.png"
           width="160" height="120" alt="Screenshot">
        </a>
      </td>
      <td>
        <a class="thumb_link" href="shots/${base}_C.htm">
          <img class="thumb_image" src="thumbs/${base}_TC.png"
           width="160" height="120" alt="Screenshot">
        </a>
      </td>
    </tr>
    <tr>
      <td>
        <a class="thumb_link" href="shots/${base}_D.htm">
          <img class="thumb_image" src="thumbs/${base}_TD.png"
           width="160" height="120" alt="Screenshot">
        </a>
      </td>
      <td>
        <a class="thumb_link" href="shots/${base}_E.htm">
          <img class="thumb_image" src="thumbs/${base}_TE.png"
           width="160" height="120" alt="Screenshot">
        </a>
      </td>
      <td>
        <a class="thumb_link" href="shots/${base}_F.htm">
          <img class="thumb_image" src="thumbs/${base}_TF.png"
           width="160" height="120" alt="Screenshot">
        </a>
      </td>
    </tr>
  </table>
</block>


<!---
  Screenshot macro
  ================

  file  : filename of image (with extension, but no path)
  title : title for the large image
  next  : filename of next page
  prev  : filename of next page
--->

<block screenshot>
  <div>
    <h2 class="page_title"> <use title> </h2>

    <img class="large_image" src="../thumbs/${file}"
       width="640" height="480" alt=${title}>
  </div>
</block>

