<!--It tests that number of 'interface' nodes is more than 1--> 
<!--If the test passes, it prints '<ok/>' else '<error/>'--> 

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:variable name="num" select="count(//interface)" /> 
  <xsl:template match="/">
    <xsl:choose>
      <xsl:when test="$num &gt; 1">
         <ok/>
      </xsl:when>
      <xsl:otherwise>
         <error/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>

