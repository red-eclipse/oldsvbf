<p id="supertext" align="center"><a href="/project"><?php echo $app['sitename']; ?></a>, <i><?php echo $app['siteblurb']; ?></i></p>
<p id="video" align="center">
	<object id="flash" type="application/x-shockwave-flash" data="<?php echo $app['sitevideo']; ?>">
		<param name="movie" value="<?php echo $app['sitevideo']; ?>" />
		<param name="allowscriptaccess" value="always" />
		<param name="allowFullScreen" value="true" />
		<embed id="flash" src="<?php echo $app['sitevideo']; ?>" type="application/x-shockwave-flash" allowfullscreen="true"></embed>
	</object>
</p>
<p id="subtext" align="center"><i><?php echo $app['siteinfo']; ?></i></p>
<p id="footer" align="center"><a href="/download">Download</a>, <a href="/wiki">Learn More</a>, <a href="/forums">Get Help</a>, or <a href="/chat">Join In</a> today!</p>
